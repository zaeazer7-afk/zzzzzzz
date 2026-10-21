#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <functional>

#include <imgui/imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );

namespace Core
{
    using fnPresent = HRESULT( __stdcall* )( IDXGISwapChain*, UINT, UINT );
    using fnResizeBuffers = HRESULT( __stdcall* )( IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT );

    class Hooks
    {
    public:

        static Hooks& Get()
        {
            static Hooks inst;
            return inst;
        }

        bool Init()
        {
            if ( m_hooked ) return true;

            DXGI_SWAP_CHAIN_DESC sd{};
            sd.BufferCount       = 1;
            sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow      = GetDesktopWindow();
            sd.SampleDesc.Count  = 1;
            sd.Windowed          = TRUE;
            sd.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

            IDXGISwapChain*  pSwap   = nullptr;
            ID3D11Device*    pDev    = nullptr;
            ID3D11DeviceContext* pCtx = nullptr;

            D3D_FEATURE_LEVEL featureLevel;
            HRESULT hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                nullptr, 0, D3D11_SDK_VERSION,
                &sd, &pSwap, &pDev, &featureLevel, &pCtx
            );
            if ( FAILED( hr ) ) return false;

            void** vtable = *reinterpret_cast<void***>( pSwap );
            m_oPresent       = reinterpret_cast<fnPresent>( vtable[8] );
            m_oResizeBuffers = reinterpret_cast<fnResizeBuffers>( vtable[13] );

            DWORD oldProt;

            VirtualProtect( &vtable[8], sizeof( void* ), PAGE_READWRITE, &oldProt );
            vtable[8] = reinterpret_cast<void*>( &HkPresent );
            VirtualProtect( &vtable[8], sizeof( void* ), oldProt, &oldProt );

            VirtualProtect( &vtable[13], sizeof( void* ), PAGE_READWRITE, &oldProt );
            vtable[13] = reinterpret_cast<void*>( &HkResizeBuffers );
            VirtualProtect( &vtable[13], sizeof( void* ), oldProt, &oldProt );

            pSwap->Release();
            pDev->Release();
            pCtx->Release();

            m_hooked = true;
            return true;
        }

        void Shutdown()
        {
            m_shuttingDown = true;

            if ( m_imguiInit )
            {
                ImGui_ImplDX11_Shutdown();
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();
                m_imguiInit = false;
            }

            if ( m_rtv ) { m_rtv->Release(); m_rtv = nullptr; }
            if ( m_device ) { m_device = nullptr; }
            if ( m_context ) { m_context = nullptr; }

            if ( m_hWnd && m_oWndProc )
            {
                SetWindowLongPtrA( m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( m_oWndProc ) );
                m_oWndProc = nullptr;
            }
        }

        using RenderFn = std::function<void()>;
        void SetRenderCallback( RenderFn fn ) { m_renderCb = fn; }

        std::atomic<bool> MenuOpen{ true };
        std::atomic<bool> m_unloadRequested{ false };

    private:

        Hooks() = default;

        static HRESULT __stdcall HkPresent( IDXGISwapChain* pSwap, UINT sync, UINT flags )
        {
            auto& self = Get();
            if ( self.m_shuttingDown )
                return self.m_oPresent( pSwap, sync, flags );

            if ( !self.m_imguiInit )
                self.InitImGui( pSwap );

            if ( self.m_imguiInit )
            {
                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();

                if ( self.m_renderCb )
                    self.m_renderCb();

                ImGui::EndFrame();
                ImGui::Render();

                self.m_context->OMSetRenderTargets( 1, &self.m_rtv, nullptr );
                ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
            }

            return self.m_oPresent( pSwap, sync, flags );
        }

        static HRESULT __stdcall HkResizeBuffers( IDXGISwapChain* pSwap, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT fl )
        {
            auto& self = Get();

            if ( self.m_rtv ) { self.m_rtv->Release(); self.m_rtv = nullptr; }

            HRESULT hr = self.m_oResizeBuffers( pSwap, count, w, h, fmt, fl );

            if ( SUCCEEDED( hr ) && self.m_device )
            {
                ID3D11Texture2D* backBuf = nullptr;
                pSwap->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void**)&backBuf );
                if ( backBuf )
                {
                    self.m_device->CreateRenderTargetView( backBuf, nullptr, &self.m_rtv );
                    backBuf->Release();
                }
            }
            return hr;
        }

        static LRESULT CALLBACK HkWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
        {
            auto& self = Get();

            if ( msg == WM_KEYDOWN && wParam == VK_INSERT )
                self.MenuOpen = !self.MenuOpen;

            if ( self.MenuOpen )
            {
                if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
                    return true;

                if ( msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST )
                    return true;
            }

            return CallWindowProcA( self.m_oWndProc, hWnd, msg, wParam, lParam );
        }

        void InitImGui( IDXGISwapChain* pSwap )
        {
            pSwap->GetDevice( __uuidof( ID3D11Device ), (void**)&m_device );
            if ( !m_device ) return;
            m_device->GetImmediateContext( &m_context );

            DXGI_SWAP_CHAIN_DESC desc;
            pSwap->GetDesc( &desc );
            m_hWnd = desc.OutputWindow;

            ID3D11Texture2D* backBuf = nullptr;
            pSwap->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void**)&backBuf );
            if ( !backBuf ) return;
            m_device->CreateRenderTargetView( backBuf, nullptr, &m_rtv );
            backBuf->Release();

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui::StyleColorsDark();
            auto& style = ImGui::GetStyle();
            style.WindowRounding   = 8.f;
            style.FrameRounding    = 4.f;
            style.GrabRounding     = 4.f;
            style.ScrollbarRounding = 6.f;
            style.WindowPadding    = ImVec2( 12.f, 12.f );
            style.FramePadding     = ImVec2( 8.f, 4.f );
            style.ItemSpacing      = ImVec2( 8.f, 6.f );

            auto* colors = style.Colors;
            colors[ ImGuiCol_WindowBg ]       = ImVec4( 0.06f, 0.06f, 0.09f, 0.94f );
            colors[ ImGuiCol_TitleBg ]        = ImVec4( 0.08f, 0.08f, 0.12f, 1.00f );
            colors[ ImGuiCol_TitleBgActive ]  = ImVec4( 0.12f, 0.12f, 0.18f, 1.00f );
            colors[ ImGuiCol_FrameBg ]        = ImVec4( 0.10f, 0.10f, 0.15f, 1.00f );
            colors[ ImGuiCol_FrameBgHovered ] = ImVec4( 0.15f, 0.15f, 0.22f, 1.00f );
            colors[ ImGuiCol_Button ]         = ImVec4( 0.20f, 0.40f, 0.90f, 0.65f );
            colors[ ImGuiCol_ButtonHovered ]  = ImVec4( 0.25f, 0.50f, 1.00f, 0.80f );
            colors[ ImGuiCol_ButtonActive ]   = ImVec4( 0.15f, 0.35f, 0.85f, 1.00f );
            colors[ ImGuiCol_Header ]         = ImVec4( 0.20f, 0.40f, 0.90f, 0.40f );
            colors[ ImGuiCol_HeaderHovered ]  = ImVec4( 0.25f, 0.50f, 1.00f, 0.50f );
            colors[ ImGuiCol_Tab ]            = ImVec4( 0.12f, 0.12f, 0.18f, 1.00f );
            colors[ ImGuiCol_TabHovered ]     = ImVec4( 0.25f, 0.50f, 1.00f, 0.80f );
            colors[ ImGuiCol_TabActive ]      = ImVec4( 0.20f, 0.40f, 0.90f, 1.00f );
            colors[ ImGuiCol_CheckMark ]      = ImVec4( 0.30f, 0.60f, 1.00f, 1.00f );
            colors[ ImGuiCol_SliderGrab ]     = ImVec4( 0.30f, 0.60f, 1.00f, 1.00f );

            ImGui_ImplWin32_Init( m_hWnd );
            ImGui_ImplDX11_Init( m_device, m_context );

            m_oWndProc = reinterpret_cast<WNDPROC>( SetWindowLongPtrA(
                m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( &HkWndProc ) ) );

            m_imguiInit = true;
        }

        bool                   m_hooked       = false;
        bool                   m_imguiInit    = false;
        std::atomic<bool>      m_shuttingDown{ false };
        fnPresent              m_oPresent       = nullptr;
        fnResizeBuffers        m_oResizeBuffers = nullptr;
        WNDPROC                m_oWndProc       = nullptr;
        HWND                   m_hWnd           = nullptr;
        ID3D11Device*          m_device         = nullptr;
        ID3D11DeviceContext*   m_context        = nullptr;
        ID3D11RenderTargetView* m_rtv           = nullptr;
        RenderFn               m_renderCb;
    };

    inline Hooks& g_Hooks = Hooks::Get();
}
