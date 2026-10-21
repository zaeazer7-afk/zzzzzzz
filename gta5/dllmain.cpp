#include "pointers.hpp"
#include "hooks/dx12.hpp"
#include "hooks/battleye.hpp"
#include "natives/invoker.hpp"
#include "natives/tables.hpp"
#include "script/fiber.hpp"
#include "menu/menu.hpp"
#include "features/self.hpp"
#include "features/vehicle.hpp"
#include "features/teleport.hpp"
#include "features/weapon.hpp"
#include "../core/memory/hooking.hpp"
#include "../core/memory/address.hpp"
#include <Windows.h>
#include <imgui.h>

namespace gta5 {

static HMODULE g_Module  = nullptr;
static bool    g_Running = true;

// Hook RunScriptThreads to get into game script context
using RunScriptThreads_t = void(*)();
static RunScriptThreads_t OrigRunScriptThreads = nullptr;

static void HookedRunScriptThreads() {
    if (OrigRunScriptThreads)
        OrigRunScriptThreads();

    script::ScriptMgr::Tick();
}

// Hook InitNativeTables to capture native handlers
using InitNativeTables_t = void(*)(void*, uint64_t);
static InitNativeTables_t OrigInitNativeTables = nullptr;

static void HookedInitNativeTables(void* table, uint64_t count) {
    auto entries = reinterpret_cast<natives::NativeRegistrationEntry*>(table);
    for (uint64_t i = 0; i < count; i++) {
        natives::NativeInvoker::RegisterHandler(entries[i].hash, entries[i].handler);
    }
    if (OrigInitNativeTables)
        OrigInitNativeTables(table, count);
}

static void FeatureTick() {
    // Runs each frame in script context via fiber
}

static DWORD WINAPI Main(LPVOID) {
    if (!Pointers::Init()) {
        MessageBoxA(nullptr, "Pattern scan failed — game version may be unsupported.", "GTA5Enhanced", MB_ICONERROR);
        FreeLibraryAndExitThread(g_Module, 1);
        return 1;
    }

    hooks::BattlEye::Init();

    core::memory::Hooking::Init();

    // Hook InitNativeTables to capture native function pointers
    if (auto* addr = Pointers::Get().InitNativeTables) {
        core::memory::Hooking::Create(
            addr, reinterpret_cast<void*>(&HookedInitNativeTables),
            reinterpret_cast<void**>(&OrigInitNativeTables));
    }

    // Hook RunScriptThreads to tick our fibers in game script context
    if (auto* addr = Pointers::Get().RunScriptThreads) {
        core::memory::Hooking::Create(
            addr, reinterpret_cast<void*>(&HookedRunScriptThreads),
            reinterpret_cast<void**>(&OrigRunScriptThreads));
    }

    script::ScriptMgr::Init();
    script::ScriptMgr::AddScript(FeatureTick, "feature_tick");

    hooks::DX12::Init();
    menu::Init();

    while (g_Running) {
        if (GetAsyncKeyState(VK_DELETE) & 1 || GetAsyncKeyState(VK_END) & 1)
            g_Running = false;
        Sleep(100);
    }

    hooks::DX12::Shutdown();
    core::memory::Hooking::Shutdown();
    script::ScriptMgr::Shutdown();
    hooks::BattlEye::Shutdown();
    Pointers::Shutdown();

    FreeLibraryAndExitThread(g_Module, 0);
    return 0;
}

} // namespace gta5

void RenderMenu() {
    gta5::menu::Render();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        gta5::g_Module = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, gta5::Main, nullptr, 0, nullptr);
    }
    return TRUE;
}
