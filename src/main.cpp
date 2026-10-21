#include "pch.h"
#include "game.h"
#include "render.h"
#include "menu.h"
#include "memory.h"
#include "battleye.h"

static std::atomic<bool> g_running = true;
static HMODULE g_module = nullptr;
static FILE* g_log = nullptr;

static void logMsg(const char* fmt, ...) {
    if (!g_log) {
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string s(path);
        auto pos = s.find_last_of("\\/");
        if (pos != std::string::npos) s = s.substr(0, pos + 1);
        s += "enhanced_log.txt";
        g_log = fopen(s.c_str(), "w");
        if (!g_log) return;
    }
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fflush(g_log);
}

static DWORD WINAPI initThread(LPVOID) {
    logMsg("[Enhanced] Init thread started\n");

    HWND hwnd = nullptr;
    for (int i = 0; i < 1200 && !hwnd; i++) {
        hwnd = FindWindowW(L"grcWindow", nullptr);
        if (!hwnd) Sleep(250);
    }
    if (!hwnd) { logMsg("[Enhanced] grcWindow not found, aborting\n"); return 0; }
    logMsg("[Enhanced] grcWindow found: %p\n", hwnd);

    logMsg("[Enhanced] Patching BattlEye...\n");
    if (battleye::init())
        logMsg("[Enhanced] BattlEye patched\n");
    else
        logMsg("[Enhanced] BattlEye patch failed (may not be present)\n");

    logMsg("[Enhanced] Waiting 15s for DX12...\n");
    Sleep(15000);

    logMsg("[Enhanced] Starting game::init()\n");
    __try {
        if (!game::init()) { logMsg("[Enhanced] game::init() returned false\n"); return 0; }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        logMsg("[Enhanced] game::init() CRASHED (0x%08X)\n", GetExceptionCode());
        return 0;
    }
    logMsg("[Enhanced] game::init() OK\n");

    logMsg("[Enhanced] Starting render::init()\n");
    __try {
        if (!render::init()) { logMsg("[Enhanced] render::init() returned false\n"); return 0; }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        logMsg("[Enhanced] render::init() CRASHED (0x%08X)\n", GetExceptionCode());
        return 0;
    }
    logMsg("[Enhanced] render::init() OK\n");

    render::setDrawCallback(menu::draw);
    render::setTickCallback(menu::tick);

    if (g_module) mem::erasePEHeaders(g_module);

    while (g_running) {
        if (GetAsyncKeyState(VK_END) & 1) g_running = false;
        Sleep(50);
    }

    render::shutdown();
    game::shutdown();

    if (g_log) { fclose(g_log); g_log = nullptr; }

    if (g_module) FreeLibraryAndExitThread(g_module, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = hModule;
        DisableThreadLibraryCalls(hModule);
        auto hThread = CreateThread(nullptr, 0, initThread, nullptr, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    return TRUE;
}
