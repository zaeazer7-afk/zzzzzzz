// dllmain.cpp — FiveM internal DLL entry point
#include "menu.h"
#include "hooks.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        g_Module = hModule;
        CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
            Hooks::Init();
            return 0;
        }, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        Hooks::Shutdown();
    }
    return TRUE;
}
