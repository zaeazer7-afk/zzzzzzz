#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace gta5::script {

// Natives MUST be called from a game script context.
// The game's TLS slot at 0x7A0 holds the active scrThread*.
// We hook a game script thread's Run vtable entry, then on each
// tick we switch to our fibers which inherit the script context.

class ScriptFiber {
public:
    using Func = std::function<void()>;

    ScriptFiber(Func func, const std::string& name, size_t stackSize = 0)
        : m_Func(std::move(func))
        , m_Name(name)
        , m_Fiber(nullptr)
        , m_Done(false) {
        m_Fiber = CreateFiber(stackSize, FiberProc, this);
    }

    ~ScriptFiber() {
        if (m_Fiber)
            DeleteFiber(m_Fiber);
    }

    ScriptFiber(const ScriptFiber&)            = delete;
    ScriptFiber& operator=(const ScriptFiber&) = delete;

    void Tick(LPVOID mainFiber) {
        if (m_Done)
            return;
        s_MainFiber = mainFiber;
        SwitchToFiber(m_Fiber);
    }

    bool IsDone() const { return m_Done; }
    const std::string& GetName() const { return m_Name; }

    // Called from within a fiber to yield back to the game thread
    static void Yield() {
        SwitchToFiber(s_MainFiber);
    }

private:
    static void CALLBACK FiberProc(LPVOID param) {
        auto self = reinterpret_cast<ScriptFiber*>(param);
        self->m_Func();
        self->m_Done = true;
        // Loop forever yielding after func completes
        while (true)
            Yield();
    }

    Func        m_Func;
    std::string m_Name;
    LPVOID      m_Fiber;
    bool        m_Done;

    inline static LPVOID s_MainFiber = nullptr;
};

class ScriptMgr {
public:
    // Convert the calling thread to a fiber (call once from hooked thread)
    static void Init() {
        s_MainFiber = ConvertThreadToFiber(nullptr);
        if (!s_MainFiber) {
            // Already a fiber (e.g. if game thread was already converted)
            s_MainFiber = GetCurrentFiber();
        }
    }

    static void Shutdown() {
        s_Fibers.clear();
        if (s_MainFiber) {
            ConvertFiberToThread();
            s_MainFiber = nullptr;
        }
    }

    // Register a script to run each tick (looping fibers)
    static void AddScript(ScriptFiber::Func func, const std::string& name, size_t stackSize = 0) {
        s_Fibers.emplace_back(std::make_unique<ScriptFiber>(
            [func]() {
                while (true) {
                    func();
                    ScriptFiber::Yield();
                }
            },
            name,
            stackSize
        ));
    }

    // Register a one-shot script
    static void AddOneShotScript(ScriptFiber::Func func, const std::string& name) {
        s_Fibers.emplace_back(std::make_unique<ScriptFiber>(func, name));
    }

    // Called from hooked game script thread each tick
    // Switches to each registered fiber, lets it execute until it yields
    static void Tick() {
        if (!s_MainFiber)
            return;

        for (auto it = s_Fibers.begin(); it != s_Fibers.end(); ) {
            auto& fiber = *it;
            fiber->Tick(s_MainFiber);
            if (fiber->IsDone())
                it = s_Fibers.erase(it);
            else
                ++it;
        }
    }

    static size_t ScriptCount() { return s_Fibers.size(); }

private:
    inline static LPVOID s_MainFiber = nullptr;
    inline static std::vector<std::unique_ptr<ScriptFiber>> s_Fibers;
};

// Convenience: call from any registered script fiber to yield one tick
inline void Yield() {
    ScriptFiber::Yield();
}

// Yield for approximately N milliseconds (yields N/16 ticks at ~60fps)
inline void Wait(uint32_t ms) {
    auto start = GetTickCount64();
    while (GetTickCount64() - start < ms)
        Yield();
}

} // namespace gta5::script
