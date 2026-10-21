#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <Windows.h>

namespace gta5::hooks {

class DX12 {
public:
    static bool Init();
    static void Shutdown();

    static IDXGISwapChain*        GetSwapChain();
    static ID3D12CommandQueue*    GetCommandQueue();

    using PresentFn       = HRESULT(WINAPI*)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    using ResizeBuffersFn = HRESULT(WINAPI*)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    using WndProcFn       = LRESULT(WINAPI*)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static PresentFn       OriginalPresent;
    static ResizeBuffersFn OriginalResizeBuffers;
    static WndProcFn       OriginalWndProc;

private:
    static HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    static LRESULT WINAPI HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static void InitImGui(IDXGISwapChain* pSwapChain);
    static void CreateRenderTargets(IDXGISwapChain* pSwapChain);
    static void CleanupRenderTargets();

    static bool                             s_Initialized;
    static HWND                             s_GameWindow;
    static ID3D12Device*                    s_Device;
    static ID3D12DescriptorHeap*            s_RtvHeap;
    static ID3D12DescriptorHeap*            s_SrvHeap;
    static ID3D12GraphicsCommandList*       s_CommandList;
    static ID3D12CommandAllocator**         s_CommandAllocators;
    static ID3D12Resource**                 s_BackBuffers;
    static D3D12_CPU_DESCRIPTOR_HANDLE*     s_RtvHandles;
    static UINT                             s_BufferCount;
};

} // namespace gta5::hooks
