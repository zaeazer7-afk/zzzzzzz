#pragma once
#include "pch.h"
#include <d3d12.h>
#include <dxgi1_4.h>

namespace render {
    bool init();
    void shutdown();

    bool isMenuOpen();
    void toggleMenu();

    ID3D12Device*       device();
    IDXGISwapChain3*    swapchain();
    HWND                gameWindow();

    using DrawFn = void(*)();
    using TickFn = void(*)();
    void setDrawCallback(DrawFn fn);
    void setTickCallback(TickFn fn);
}
