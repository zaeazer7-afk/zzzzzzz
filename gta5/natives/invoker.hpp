#pragma once
#include <cstdint>
#include <cstring>
#include <unordered_map>

namespace gta5::natives {

// Matches RAGE engine's scrNativeCallContext layout
struct scrNativeCallContext {
    uint64_t* m_ReturnValue;   // 0x00
    uint32_t  m_ArgCount;      // 0x08
    uint32_t  _pad0;           // 0x0C
    uint64_t* m_Args;          // 0x10
    uint32_t  m_DataCount;     // 0x18
    uint32_t  _pad1;           // 0x1C
    // Vector3 translation buffer follows in-engine; not needed for invocation

    template <typename T>
    T GetArg(uint32_t index) const {
        static_assert(sizeof(T) <= 8);
        return *reinterpret_cast<T*>(&m_Args[index]);
    }

    template <typename T>
    void SetResult(T value) {
        static_assert(sizeof(T) <= 8);
        *reinterpret_cast<T*>(m_ReturnValue) = value;
    }

    template <typename T>
    T GetResult() const {
        static_assert(sizeof(T) <= 8);
        return *reinterpret_cast<T*>(m_ReturnValue);
    }
};

using NativeHandler = void(*)(scrNativeCallContext*);

class NativeInvoker {
public:
    static constexpr uint32_t MAX_ARGS = 32;

    static void BeginCall() {
        s_ArgCount = 0;
        std::memset(s_Args, 0, sizeof(s_Args));
        s_ReturnValue = 0;
        s_Context.m_ReturnValue = &s_ReturnValue;
        s_Context.m_Args        = s_Args;
        s_Context.m_DataCount   = 0;
    }

    template <typename T>
    static void Push(T value) {
        static_assert(sizeof(T) <= 8);
        if (s_ArgCount >= MAX_ARGS)
            return;
        *reinterpret_cast<T*>(&s_Args[s_ArgCount]) = value;
        s_ArgCount++;
        s_Context.m_ArgCount = s_ArgCount;
    }

    // Call after pushing all args. Returns false if hash not found.
    static bool EndCall(uint64_t nativeHash) {
        auto it = s_Handlers.find(nativeHash);
        if (it == s_Handlers.end())
            return false;

        auto handler = reinterpret_cast<NativeHandler>(it->second);
        handler(&s_Context);
        return true;
    }

    template <typename T>
    static T GetResult() {
        return s_Context.GetResult<T>();
    }

    // One-shot invoke helpers
    template <typename R, typename... Args>
    static R Invoke(uint64_t hash, Args... args) {
        BeginCall();
        (Push(args), ...);
        EndCall(hash);
        if constexpr (!std::is_void_v<R>)
            return GetResult<R>();
    }

    // Called by NativeTable hook during game init
    static void RegisterHandler(uint64_t hash, void* handler) {
        s_Handlers[hash] = handler;
    }

    static bool HasHandler(uint64_t hash) {
        return s_Handlers.count(hash) > 0;
    }

    static size_t HandlerCount() {
        return s_Handlers.size();
    }

    static void ClearHandlers() {
        s_Handlers.clear();
    }

    static std::unordered_map<uint64_t, void*>& GetHandlers() {
        return s_Handlers;
    }

private:
    inline static uint64_t               s_Args[MAX_ARGS] = {};
    inline static uint64_t               s_ReturnValue     = 0;
    inline static uint32_t               s_ArgCount        = 0;
    inline static scrNativeCallContext    s_Context         = {};
    inline static std::unordered_map<uint64_t, void*> s_Handlers;
};

} // namespace gta5::natives
