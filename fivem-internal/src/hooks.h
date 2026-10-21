#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

inline HMODULE g_Module = nullptr;

namespace Hooks {
    void Init();
    void Shutdown();
}
