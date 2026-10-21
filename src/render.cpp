#include "render.h"
#include "game.h"
#include "memory.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {
    constexpr int MAX_BACK_BUFFERS    = 4;
    constexpr int NUM_FRAMES_IN_FLIGHT = 3;

    bool                    g_open       = false;
    bool                    g_init       = false;
    HWND                    g_hwnd       = nullptr;
    ID3D12Device*           g_device     = nullptr;
    IDXGISwapChain3*        g_swapchain  = nullptr;

    ID3D12CommandQueue*         g_ownQueue   = nullptr;
    ID3D12DescriptorHeap*       g_srvHeap    = nullptr;
    ID3D12DescriptorHeap*       g_rtvHeap    = nullptr;
    ID3D12CommandAllocator*     g_cmdAlloc[MAX_BACK_BUFFERS] = {};
    ID3D12GraphicsCommandList*  g_cmdList    = nullptr;
    ID3D12Resource*             g_backBuffers[MAX_BACK_BUFFERS] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE g_rtvHandles[MAX_BACK_BUFFERS]  = {};
    UINT                        g_rtvDescSize = 0;
    UINT                        g_bufferCount = 0;

    ID3D12Fence*  g_fence       = nullptr;
    HANDLE        g_fenceEvent  = nullptr;
    UINT64        g_fenceValues[MAX_BACK_BUFFERS] = {};
    UINT64        g_nextFence   = 1;

    void**   g_origVtable   = nullptr;
    void**   g_cloneVtable  = nullptr;
    size_t   g_vtableSize   = 0;
    WNDPROC  g_origWndProc  = nullptr;

    render::DrawFn g_drawCb = nullptr;
    render::TickFn g_tickCb = nullptr;

    using PresentFn       = HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    PresentFn       g_origPresent       = nullptr;
    ResizeBuffersFn g_origResizeBuffers = nullptr;

    void cleanupRenderTargets() {
        for (UINT i = 0; i < MAX_BACK_BUFFERS; i++) {
            if (g_backBuffers[i]) { g_backBuffers[i]->Release(); g_backBuffers[i] = nullptr; }
        }
    }

    void createRenderTargets() {
        DXGI_SWAP_CHAIN_DESC desc;
        g_swapchain->GetDesc(&desc);
        g_bufferCount = desc.BufferCount;
        if (g_bufferCount > MAX_BACK_BUFFERS) g_bufferCount = MAX_BACK_BUFFERS;

        auto rtvStart = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < g_bufferCount; i++) {
            g_backBuffers[i] = nullptr;
            g_swapchain->GetBuffer(i, IID_PPV_ARGS(&g_backBuffers[i]));
            g_rtvHandles[i] = rtvStart;
            g_rtvHandles[i].ptr += (SIZE_T)(i * g_rtvDescSize);
            g_device->CreateRenderTargetView(g_backBuffers[i], nullptr, g_rtvHandles[i]);
        }
    }

    void waitForGpu() {
        if (!g_ownQueue || !g_fence || !g_fenceEvent) return;
        auto val = g_nextFence++;
        g_ownQueue->Signal(g_fence, val);
        if (g_fence->GetCompletedValue() < val) {
            g_fence->SetEventOnCompletion(val, g_fenceEvent);
            WaitForSingleObject(g_fenceEvent, 5000);
        }
    }

    void initDX12Resources() {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_ownQueue));

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_srvHeap));
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.NumDescriptors = MAX_BACK_BUFFERS;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_rtvHeap));
            g_rtvDescSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        DXGI_SWAP_CHAIN_DESC scDesc;
        g_swapchain->GetDesc(&scDesc);
        g_bufferCount = scDesc.BufferCount;
        if (g_bufferCount > MAX_BACK_BUFFERS) g_bufferCount = MAX_BACK_BUFFERS;

        for (UINT i = 0; i < g_bufferCount; i++) {
            g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAlloc[i]));
        }

        g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            g_cmdAlloc[0], nullptr, IID_PPV_ARGS(&g_cmdList));
        g_cmdList->Close();

        g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
        g_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        createRenderTargets();
    }

    void initImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;

        auto& style = ImGui::GetStyle();
        ImGui::StyleColorsDark(&style);
        style.WindowRounding    = 6.f;
        style.FrameRounding     = 4.f;
        style.GrabRounding      = 3.f;
        style.ScrollbarRounding = 4.f;
        style.WindowBorderSize  = 1.f;
        style.FrameBorderSize   = 0.f;

        auto* c = style.Colors;
        c[ImGuiCol_WindowBg]        = ImVec4(0.06f, 0.06f, 0.10f, 0.94f);
        c[ImGuiCol_TitleBg]         = ImVec4(0.10f, 0.08f, 0.18f, 1.00f);
        c[ImGuiCol_TitleBgActive]   = ImVec4(0.16f, 0.10f, 0.30f, 1.00f);
        c[ImGuiCol_FrameBg]         = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
        c[ImGuiCol_FrameBgHovered]  = ImVec4(0.20f, 0.18f, 0.30f, 1.00f);
        c[ImGuiCol_FrameBgActive]   = ImVec4(0.26f, 0.20f, 0.40f, 1.00f);
        c[ImGuiCol_Button]          = ImVec4(0.30f, 0.15f, 0.55f, 1.00f);
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.40f, 0.22f, 0.70f, 1.00f);
        c[ImGuiCol_ButtonActive]    = ImVec4(0.50f, 0.28f, 0.85f, 1.00f);
        c[ImGuiCol_CheckMark]       = ImVec4(0.70f, 0.40f, 1.00f, 1.00f);
        c[ImGuiCol_SliderGrab]      = ImVec4(0.50f, 0.28f, 0.80f, 1.00f);
        c[ImGuiCol_SliderGrabActive]= ImVec4(0.65f, 0.38f, 0.95f, 1.00f);
        c[ImGuiCol_Header]          = ImVec4(0.20f, 0.12f, 0.35f, 1.00f);
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.30f, 0.18f, 0.50f, 1.00f);
        c[ImGuiCol_HeaderActive]    = ImVec4(0.38f, 0.24f, 0.65f, 1.00f);
        c[ImGuiCol_Tab]             = ImVec4(0.16f, 0.10f, 0.28f, 1.00f);
        c[ImGuiCol_TabHovered]      = ImVec4(0.35f, 0.20f, 0.60f, 1.00f);
        c[ImGuiCol_TabActive]       = ImVec4(0.28f, 0.16f, 0.48f, 1.00f);
        c[ImGuiCol_Separator]       = ImVec4(0.30f, 0.20f, 0.50f, 0.50f);

        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX12_Init(g_device, NUM_FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM,
            g_srvHeap,
            g_srvHeap->GetCPUDescriptorHandleForHeapStart(),
            g_srvHeap->GetGPUDescriptorHandleForHeapStart());

        g_init = true;
    }

    LRESULT CALLBACK hookedWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_KEYDOWN && wp == VK_INSERT)
            g_open = !g_open;

        if (g_open) {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
            switch (msg) {
                case WM_LBUTTONDOWN: case WM_LBUTTONUP:
                case WM_RBUTTONDOWN: case WM_RBUTTONUP:
                case WM_MBUTTONDOWN: case WM_MBUTTONUP:
                case WM_MOUSEWHEEL:  case WM_MOUSEMOVE:
                case WM_KEYDOWN:     case WM_KEYUP:
                case WM_CHAR:        case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                    if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard)
                        return TRUE;
            }
        }
        return CallWindowProcW(g_origWndProc, hwnd, msg, wp, lp);
    }

    HRESULT WINAPI hookedPresent(IDXGISwapChain3* sc, UINT sync, UINT flags) {
        if (!g_init) {
            g_swapchain = sc;

            if (SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&g_device)))) {
                initDX12Resources();

                g_hwnd = FindWindowW(L"grcWindow", nullptr);
                if (!g_hwnd) {
                    DXGI_SWAP_CHAIN_DESC desc;
                    sc->GetDesc(&desc);
                    g_hwnd = desc.OutputWindow;
                }

                initImGui();
                g_origWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hookedWndProc);
            }
        }

        if (!g_init || !g_ownQueue) return g_origPresent(sc, sync, flags);

        if (g_tickCb) g_tickCb();

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::GetIO().MouseDrawCursor = g_open;

        if (g_open && g_drawCb) g_drawCb();

        ImGui::EndFrame();
        ImGui::Render();

        UINT bufIdx = g_swapchain->GetCurrentBackBufferIndex();
        if (bufIdx >= g_bufferCount) bufIdx = 0;

        if (g_fence->GetCompletedValue() < g_fenceValues[bufIdx]) {
            g_fence->SetEventOnCompletion(g_fenceValues[bufIdx], g_fenceEvent);
            WaitForSingleObject(g_fenceEvent, 5000);
        }

        g_cmdAlloc[bufIdx]->Reset();
        g_cmdList->Reset(g_cmdAlloc[bufIdx], nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = g_backBuffers[bufIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_cmdList->ResourceBarrier(1, &barrier);

        g_cmdList->OMSetRenderTargets(1, &g_rtvHandles[bufIdx], FALSE, nullptr);
        g_cmdList->SetDescriptorHeaps(1, &g_srvHeap);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_cmdList);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        g_cmdList->ResourceBarrier(1, &barrier);

        g_cmdList->Close();

        ID3D12CommandList* ppCmdLists[] = { g_cmdList };
        g_ownQueue->ExecuteCommandLists(1, ppCmdLists);

        g_fenceValues[bufIdx] = g_nextFence++;
        g_ownQueue->Signal(g_fence, g_fenceValues[bufIdx]);

        return g_origPresent(sc, sync, flags);
    }

    HRESULT WINAPI hookedResizeBuffers(IDXGISwapChain3* sc, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
        waitForGpu();
        cleanupRenderTargets();

        if (g_init) ImGui_ImplDX12_InvalidateDeviceObjects();

        auto hr = g_origResizeBuffers(sc, count, w, h, fmt, flags);

        if (g_init) {
            DXGI_SWAP_CHAIN_DESC desc;
            sc->GetDesc(&desc);
            g_bufferCount = desc.BufferCount;
            if (g_bufferCount > MAX_BACK_BUFFERS) g_bufferCount = MAX_BACK_BUFFERS;

            for (UINT i = 0; i < g_bufferCount; i++) {
                if (!g_cmdAlloc[i])
                    g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAlloc[i]));
            }

            createRenderTargets();
            ImGui_ImplDX12_CreateDeviceObjects();
        }
        return hr;
    }

    size_t countVtable(void** vtable) {
        size_t count = 0;
        MEMORY_BASIC_INFORMATION mbi;
        while (VirtualQuery(vtable[count], &mbi, sizeof(mbi))) {
            if (!(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
                break;
            count++;
            if (count > 512) break;
        }
        return count;
    }
}

bool render::init() {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, DefWindowProcW, 0, 0, GetModuleHandleW(nullptr),
                        nullptr, nullptr, nullptr, nullptr, L"EnhDummy", nullptr };
    RegisterClassExW(&wc);
    auto hwnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                                0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    IDXGIFactory4* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    ID3D12Device* dev = nullptr;
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev)))) {
        factory->Release();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ID3D12CommandQueue* cmdQueue = nullptr;
    dev->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));

    DXGI_SWAP_CHAIN_DESC1 scd = {};
    scd.BufferCount  = 2;
    scd.Width        = 100;
    scd.Height       = 100;
    scd.Format       = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.SampleDesc.Count = 1;

    IDXGISwapChain1* sc1 = nullptr;
    if (FAILED(factory->CreateSwapChainForHwnd(cmdQueue, hwnd, &scd, nullptr, nullptr, &sc1))) {
        cmdQueue->Release(); dev->Release(); factory->Release();
        DestroyWindow(hwnd); UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    IDXGISwapChain3* sc = nullptr;
    sc1->QueryInterface(IID_PPV_ARGS(&sc));
    sc1->Release();

    g_origVtable = *(void***)sc;
    g_vtableSize = countVtable(g_origVtable);
    g_origPresent       = (PresentFn)g_origVtable[8];
    g_origResizeBuffers = (ResizeBuffersFn)g_origVtable[13];

    g_cloneVtable = new void*[g_vtableSize];
    memcpy(g_cloneVtable, g_origVtable, g_vtableSize * sizeof(void*));
    g_cloneVtable[8]  = (void*)hookedPresent;
    g_cloneVtable[13] = (void*)hookedResizeBuffers;

    sc->Release();
    cmdQueue->Release();
    dev->Release();
    factory->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    IDXGISwapChain3* gameSc = nullptr;
    for (int tries = 0; tries < 600 && !gameSc; tries++) {
        auto addr = mem::scanIDA(sigs::SWAPCHAIN);
        if (addr) {
            auto scPtr = mem::rip(addr);
            auto rawSc = mem::read<IDXGISwapChain*>(scPtr);
            if (rawSc) rawSc->QueryInterface(IID_PPV_ARGS(&gameSc));
        }
        if (!gameSc) Sleep(100);
    }
    if (!gameSc) return false;

    auto scVtablePtr = (void***)gameSc;
    DWORD old;
    VirtualProtect(scVtablePtr, sizeof(void*), PAGE_READWRITE, &old);
    *scVtablePtr = g_cloneVtable;
    VirtualProtect(scVtablePtr, sizeof(void*), old, &old);

    g_swapchain = gameSc;
    return true;
}

void render::shutdown() {
    waitForGpu();

    if (g_origWndProc && g_hwnd)
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);

    if (g_init) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    cleanupRenderTargets();

    for (int i = 0; i < MAX_BACK_BUFFERS; i++) {
        if (g_cmdAlloc[i]) { g_cmdAlloc[i]->Release(); g_cmdAlloc[i] = nullptr; }
    }
    if (g_cmdList)   { g_cmdList->Release();   g_cmdList = nullptr; }
    if (g_srvHeap)   { g_srvHeap->Release();   g_srvHeap = nullptr; }
    if (g_rtvHeap)   { g_rtvHeap->Release();   g_rtvHeap = nullptr; }
    if (g_fence)     { g_fence->Release();     g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_ownQueue)  { g_ownQueue->Release();  g_ownQueue = nullptr; }

    delete[] g_cloneVtable;
    g_cloneVtable = nullptr;
}

bool render::isMenuOpen()  { return g_open; }
void render::toggleMenu()  { g_open = !g_open; }

ID3D12Device*        render::device()     { return g_device; }
IDXGISwapChain3*     render::swapchain()  { return g_swapchain; }
HWND                 render::gameWindow() { return g_hwnd; }

void render::setDrawCallback(DrawFn fn) { g_drawCb = fn; }
void render::setTickCallback(TickFn fn) { g_tickCb = fn; }
