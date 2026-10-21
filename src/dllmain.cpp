#include <Windows.h>
#include <thread>
#include <chrono>

#include <core/anti_detection.hpp>
#include <core/config.hpp>
#include <core/scanner.hpp>
#include <core/lua_runtime.hpp>
#include <core/scheduler.hpp>
#include <core/hooks.hpp>

#include <features/esp.hpp>
#include <features/aimbot.hpp>
#include <features/combat.hpp>
#include <features/movement.hpp>
#include <features/freecam.hpp>
#include <features/vehicle_features.hpp>
#include <features/troll.hpp>
#include <features/world.hpp>
#include <features/protection.hpp>
#include <features/teleport.hpp>
#include <features/network.hpp>
#include <features/hud.hpp>
#include <features/weapon_mods.hpp>
#include <features/ped_features.hpp>
#include <features/spoofing.hpp>
#include <features/recovery.hpp>

#include <menu/menu.hpp>

static HMODULE g_hModule = nullptr;

static void MainThread( LPVOID param )
{
    g_hModule = static_cast<HMODULE>( param );

    // 1. Anti-detection first
    Core::g_AntiDetection.Init( g_hModule );

    // 2. Load config
    Core::g_Config.Init();

    // 3. Wait for Lua runtime
    bool luaReady = false;
    for ( int i = 0; i < 120; i++ )
    {
        if ( Core::g_LuaRuntime.Init() )
        {
            luaReady = true;
            break;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }

    // 4. Hook D3D11 Present
    bool hooked = false;
    for ( int i = 0; i < 60; i++ )
    {
        if ( Core::g_Hooks.Init() )
        {
            hooked = true;
            break;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }

    if ( !hooked )
    {
        FreeLibraryAndExitThread( g_hModule, 0 );
        return;
    }

    // 5. Set render callback — called every frame from HkPresent
    Core::g_Hooks.SetRenderCallback( []()
    {
        // Process queued Lua executions
        Core::g_Scheduler.ProcessQueue();

        // Update active features
        if ( Core::g_LuaRuntime.IsReady() )
        {
            Features::g_ESP.Update();
            Features::g_Aimbot.Update();
            Features::g_Combat.Update();
            Features::g_Movement.Update();
            Features::g_Freecam.Update();
            Features::g_VehicleFeatures.Update();
            Features::g_WorldFeatures.Update();
            Features::g_Protection.Update();
            Features::g_Teleport.Update();
            Features::g_Network.Update();
            Features::g_WeaponMods.Update();
            Features::g_PedFeatures.Update();
            Features::g_Spoofing.Update();
        }

        Features::g_HUD.Update();

        Menu::g_MainMenu.Render();

        if ( Menu::g_MainMenu.m_requestUnload )
            Core::g_Hooks.m_unloadRequested = true;
    } );

    // 6. Start background integrity monitor
    Core::g_AntiDetection.StartIntegrityMonitor();

    // 7. Spin until unload requested
    while ( !Core::g_Hooks.m_unloadRequested )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

    // 8. Save config before exit
    Core::g_Config.SyncToFile();
    Core::g_AntiDetection.StopIntegrityMonitor();
    Core::g_Hooks.Shutdown();

    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    FreeLibraryAndExitThread( g_hModule, 0 );
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD reason, LPVOID )
{
    if ( reason == DLL_PROCESS_ATTACH )
    {
        DisableThreadLibraryCalls( hModule );
        HANDLE hThread = CreateThread( nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>( MainThread ),
            hModule, 0, nullptr );
        if ( hThread )
            CloseHandle( hThread );
    }
    return TRUE;
}
