#include "dx12.hpp"
#include "../pointers.hpp"
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <MinHook.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace gta5::hooks {

DX12::PresentFn       DX12::OriginalPresent       = nullptr;
DX12::ResizeBuffersFn DX12::OriginalResizeBuffers  = nullptr;
DX12::WndProcFn       DX12::OriginalWndProc        = nullptr;

bool                             DX12::s_Initialized      = false;
HWND                             DX12::s_GameWindow        = nullptr;
ID3D12Device*                    DX12::s_Device            = nullptr;
ID3D12DescriptorHeap*            DX12::s_RtvHeap           = nullptr;
ID3D12DescriptorHeap*            DX12::s_SrvHeap           = nullptr;
ID3D12GraphicsCommandList*       DX12::s_CommandList        = nullptr;
ID3D12CommandAllocator**         DX12::s_CommandAllocators  = nullptr;
ID3D12Resource**                 DX12::s_BackBuffers        = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE*     DX12::s_RtvHandles        = nullptr;
UINT                             DX12::s_BufferCount        = 0;

bool DX12::Init() {
    auto* swapChain = GetSwapChain();
    if (!swapChain)
        return false;

    void** vtable = *reinterpret_cast<void***>(swapChain);

    MH_CreateHook(vtable[8], &HookedPresent, reinterpret_cast<void**>(&OriginalPresent));
    MH_CreateHook(vtable[13], &HookedResizeBuffers, reinterpret_cast<void**>(&OriginalResizeBuffers));
    MH_EnableHook(vtable[8]);
    MH_EnableHook(vtable[13]);

    auto* wndProcAddr = Pointers::Get().WndProc;
    if (wndProcAddr) {
        MH_CreateHook(wndProcAddr, &HookedWndProc, reinterpret_cast<void**>(&OriginalWndProc));
        MH_EnableHook(wndProcAddr);
    }

    return true;
}

void DX12::Shutdown() {
    if (s_Initialized) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    CleanupRenderTargets();

    if (s_SrvHeap) { s_SrvHeap->Release(); s_SrvHeap = nullptr; }
    if (s_RtvHeap) { s_RtvHeap->Release(); s_RtvHeap = nullptr; }
    if (s_CommandList) { s_CommandList->Release(); s_CommandList = nullptr; }

    if (s_CommandAllocators) {
        for (UINT i = 0; i < s_BufferCount; i++)
            if (s_CommandAllocators[i]) s_CommandAllocators[i]->Release();
        delete[] s_CommandAllocators;
        s_CommandAllocators = nullptr;
    }

    s_Initialized = false;
}

IDXGISwapChain* DX12::GetSwapChain() {
    return Pointers::Get().SwapChain;
}

ID3D12CommandQueue* DX12::GetCommandQueue() {
    return Pointers::Get().CommandQueue;
}

void DX12::InitImGui(IDXGISwapChain* pSwapChain) {
    DXGI_SWAP_CHAIN_DESC desc{};
    pSwapChain->GetDesc(&desc);
    s_GameWindow = desc.OutputWindow;
    s_BufferCount = desc.BufferCount;

    pSwapChain->GetDevice(IID_PPV_ARGS(&s_Device));

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = s_BufferCount;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    s_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&s_RtvHeap));

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    s_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&s_SrvHeap));

    s_CommandAllocators = new ID3D12CommandAllocator*[s_BufferCount]{};
    for (UINT i = 0; i < s_BufferCount; i++)
        s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_CommandAllocators[i]));

    s_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_CommandAllocators[0], nullptr, IID_PPV_ARGS(&s_CommandList));
    s_CommandList->Close();

    CreateRenderTargets(pSwapChain);

    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(s_GameWindow);
    ImGui_ImplDX12_Init(
        s_Device,
        static_cast<int>(s_BufferCount),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        s_SrvHeap,
        s_SrvHeap->GetCPUDescriptorHandleForHeapStart(),
        s_SrvHeap->GetGPUDescriptorHandleForHeapStart()
    );

    ImGui::StyleColorsDark();
    s_Initialized = true;
}

void DX12::CreateRenderTargets(IDXGISwapChain* pSwapChain) {
    s_BackBuffers = new ID3D12Resource*[s_BufferCount]{};
    s_RtvHandles = new D3D12_CPU_DESCRIPTOR_HANDLE[s_BufferCount]{};

    auto rtvIncrement = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto rtvStart = s_RtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < s_BufferCount; i++) {
        pSwapChain->GetBuffer(i, IID_PPV_ARGS(&s_BackBuffers[i]));
        s_RtvHandles[i].ptr = rtvStart.ptr + static_cast<SIZE_T>(i) * rtvIncrement;
        s_Device->CreateRenderTargetView(s_BackBuffers[i], nullptr, s_RtvHandles[i]);
    }
}

void DX12::CleanupRenderTargets() {
    if (s_BackBuffers) {
        for (UINT i = 0; i < s_BufferCount; i++)
            if (s_BackBuffers[i]) { s_BackBuffers[i]->Release(); s_BackBuffers[i] = nullptr; }
        delete[] s_BackBuffers;
        s_BackBuffers = nullptr;
    }
    delete[] s_RtvHandles;
    s_RtvHandles = nullptr;
}

HRESULT WINAPI DX12::HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!s_Initialized)
        InitImGui(pSwapChain);

    auto* commandQueue = GetCommandQueue();
    if (!commandQueue || !s_Initialized)
        return OriginalPresent(pSwapChain, SyncInterval, Flags);

    auto* swapChain3 = static_cast<IDXGISwapChain3*>(pSwapChain);
    UINT bufferIndex = swapChain3->GetCurrentBackBufferIndex();

    auto* allocator = s_CommandAllocators[bufferIndex];
    allocator->Reset();
    s_CommandList->Reset(allocator, nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = s_BackBuffers[bufferIndex];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    s_CommandList->ResourceBarrier(1, &barrier);

    s_CommandList->OMSetRenderTargets(1, &s_RtvHandles[bufferIndex], FALSE, nullptr);
    s_CommandList->SetDescriptorHeaps(1, &s_SrvHeap);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Menu rendering happens here via the GUI module
    extern void RenderMenu();
    RenderMenu();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), s_CommandList);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    s_CommandList->ResourceBarrier(1, &barrier);

    s_CommandList->Close();
    ID3D12CommandList* lists[] = { s_CommandList };
    commandQueue->ExecuteCommandLists(1, lists);

    return OriginalPresent(pSwapChain, SyncInterval, Flags);
}

HRESULT WINAPI DX12::HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    CleanupRenderTargets();

    if (s_Initialized) {
        ImGui_ImplDX12_InvalidateDeviceObjects();
    }

    auto hr = OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    if (SUCCEEDED(hr) && s_Initialized) {
        s_BufferCount = BufferCount;
        CreateRenderTargets(pSwapChain);
        ImGui_ImplDX12_CreateDeviceObjects();
    }

    return hr;
}

LRESULT WINAPI DX12::HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (s_Initialized) {
        if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
            auto& io = ImGui::GetIO();
            io.MouseDrawCursor = !io.MouseDrawCursor;
        }

        if (ImGui::GetIO().MouseDrawCursor) {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return TRUE;
        }
    }

    return OriginalWndProc(hWnd, msg, wParam, lParam);
}

} // namespace gta5::hooks
