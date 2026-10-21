// hooks.cpp — D3D11 Present hook via vtable swap
#include "hooks.h"
#include "menu.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <mutex>

// ---- state ----
static ID3D11Device*           g_Device        = nullptr;
static ID3D11DeviceContext*    g_Context       = nullptr;
static IDXGISwapChain*         g_SwapChain     = nullptr;
static ID3D11RenderTargetView* g_RenderTarget  = nullptr;
static HWND                    g_Window        = nullptr;
static WNDPROC                 g_OrigWndProc   = nullptr;
static bool                    g_Initialized   = false;
static bool                    g_Shutdown       = false;

// ---- original function pointers ----
typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(__stdcall* ResizeBuffersFn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
static PresentFn        oPresent        = nullptr;
static ResizeBuffersFn  oResizeBuffers  = nullptr;

// ---- forward ----
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (Menu::IsOpen() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return 1;

    if (Menu::IsOpen()) {
        switch (msg) {
        case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: case WM_MOUSEWHEEL: case WM_KEYDOWN:
        case WM_KEYUP: case WM_CHAR:
            return 1;
        }
    }
    return CallWindowProcW(g_OrigWndProc, hwnd, msg, wp, lp);
}

static void InitImGui(IDXGISwapChain* sc) {
    DXGI_SWAP_CHAIN_DESC desc;
    sc->GetDesc(&desc);
    g_Window = desc.OutputWindow;

    sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device);
    g_Device->GetImmediateContext(&g_Context);

    ID3D11Texture2D* backbuf = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuf);
    g_Device->CreateRenderTargetView(backbuf, nullptr, &g_RenderTarget);
    backbuf->Release();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    Menu::StyleInit();

    ImGui_ImplWin32_Init(g_Window);
    ImGui_ImplDX11_Init(g_Device, g_Context);

    g_OrigWndProc = (WNDPROC)SetWindowLongPtrW(g_Window, GWLP_WNDPROC, (LONG_PTR)WndProc);
    g_Initialized = true;
}

static HRESULT __stdcall hkPresent(IDXGISwapChain* sc, UINT syncInterval, UINT flags) {
    if (g_Shutdown) return oPresent(sc, syncInterval, flags);

    if (!g_Initialized)
        InitImGui(sc);

    if (GetAsyncKeyState(VK_INSERT) & 1)
        Menu::Toggle();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Menu::Render();

    ImGui::EndFrame();
    ImGui::Render();
    g_Context->OMSetRenderTargets(1, &g_RenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(sc, syncInterval, flags);
}

static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* sc, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT fl) {
    if (g_RenderTarget) { g_RenderTarget->Release(); g_RenderTarget = nullptr; }

    HRESULT hr = oResizeBuffers(sc, count, w, h, fmt, fl);

    ID3D11Texture2D* backbuf = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuf);
    if (backbuf) {
        g_Device->CreateRenderTargetView(backbuf, nullptr, &g_RenderTarget);
        backbuf->Release();
    }
    return hr;
}

// ---- vtable hook via dummy swap chain ----
void Hooks::Init() {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, DefWindowProcW, 0, 0,
                       GetModuleHandleW(nullptr), nullptr, nullptr, nullptr,
                       nullptr, L"DX", nullptr };
    RegisterClassExW(&wc);
    HWND dummy = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                                  0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = dummy;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    D3D_FEATURE_LEVEL fl;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                   nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, nullptr);

    void** vtable = *reinterpret_cast<void***>(sc);
    oPresent       = reinterpret_cast<PresentFn>(vtable[8]);
    oResizeBuffers = reinterpret_cast<ResizeBuffersFn>(vtable[13]);

    sc->Release();
    dev->Release();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    // patch vtable — in production use MinHook or similar
    DWORD old;
    VirtualProtect(&vtable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    vtable[8] = reinterpret_cast<void*>(hkPresent);
    VirtualProtect(&vtable[8], sizeof(void*), old, &old);

    VirtualProtect(&vtable[13], sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    vtable[13] = reinterpret_cast<void*>(hkResizeBuffers);
    VirtualProtect(&vtable[13], sizeof(void*), old, &old);
}

void Hooks::Shutdown() {
    g_Shutdown = true;
    Sleep(100);

    if (g_Initialized) {
        SetWindowLongPtrW(g_Window, GWLP_WNDPROC, (LONG_PTR)g_OrigWndProc);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    if (g_RenderTarget) g_RenderTarget->Release();
    if (g_Context) g_Context->Release();
    if (g_Device) g_Device->Release();
}
