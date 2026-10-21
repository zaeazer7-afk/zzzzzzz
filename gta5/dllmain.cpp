#include "pointers.hpp"
#include "hooks/dx12.hpp"
#include "hooks/battleye.hpp"
#include "../core/memory/module.hpp"
#include "../core/memory/hooking.hpp"
#include <Windows.h>

namespace gta5 {

static HMODULE g_Module = nullptr;
static bool    g_Running = true;

void RenderMenu();

static DWORD WINAPI Main(LPVOID lpParam) {
    // 1. Resolve game module
    core::memory::Module::Init("GTA5_Enhanced.exe");

    // 2. Pattern scan all pointers
    if (!Pointers::Init()) {
        MessageBoxA(nullptr, "Pattern scan failed — game version may be unsupported.", "GTA5Enhanced", MB_ICONERROR);
        FreeLibraryAndExitThread(g_Module, 1);
        return 1;
    }

    // 3. Patch BattlEye
    hooks::BattlEye::Init();

    // 4. Initialize MinHook
    core::memory::Hooking::Init();

    // 5. Hook DX12 swapchain for overlay
    hooks::DX12::Init();

    // 6. Main loop — hold until unload key
    while (g_Running) {
        if (GetAsyncKeyState(VK_DELETE) & 1 || GetAsyncKeyState(VK_END) & 1)
            g_Running = false;
        Sleep(100);
    }

    // 7. Cleanup in reverse
    hooks::DX12::Shutdown();
    core::memory::Hooking::Shutdown();
    hooks::BattlEye::Shutdown();
    Pointers::Shutdown();
    core::memory::Module::Shutdown();

    FreeLibraryAndExitThread(g_Module, 0);
    return 0;
}

} // namespace gta5

void RenderMenu() {
    // Placeholder — filled by the trainer/gui module
    static bool show = true;
    if (!show) return;

    ImGui::SetNextWindowSize(ImVec2(420, 320), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("GTA5 Enhanced", &show)) {
        ImGui::Text("Press INSERT to toggle menu");
        ImGui::Separator();
        ImGui::Text("DELETE / END to unload");
    }
    ImGui::End();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        gta5::g_Module = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, gta5::Main, nullptr, 0, nullptr);
    }
    return TRUE;
}
