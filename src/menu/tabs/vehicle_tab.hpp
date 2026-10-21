#pragma once

#include <imgui.h>
#include <features/vehicle_features.hpp>

namespace Menu::Tabs
{
    inline void RenderVehicle()
    {
        auto& v = Features::VehicleFeatures::Get();

        ImGui::SeparatorText( "Protection" );
        ImGui::Checkbox( "Vehicle God Mode", &v.VehicleGodMode );
        ImGui::Checkbox( "Seatbelt", &v.Seatbelt );

        ImGui::Spacing();
        ImGui::SeparatorText( "Speed" );

        ImGui::Checkbox( "Horn Boost", &v.HornBoost );
        if ( v.HornBoost )
            ImGui::SliderFloat( "Boost Force", &v.HornBoostForce, 10.f, 200.f, "%.0f" );

        ImGui::Checkbox( "Speed Boost", &v.SpeedBoost );
        if ( v.SpeedBoost )
            ImGui::SliderFloat( "Boost Amount", &v.SpeedBoostAmount, 5.f, 100.f, "%.0f" );

        ImGui::Checkbox( "Infinite Nitro", &v.InfiniteNitro );

        ImGui::Spacing();
        ImGui::SeparatorText( "Handling" );

        ImGui::Checkbox( "Drift Mode", &v.DriftMode );
        ImGui::Checkbox( "Sticky Wheels", &v.StickyWheels );
        ImGui::Checkbox( "Auto-Pilot", &v.AutoPilot );

        ImGui::Spacing();
        ImGui::SeparatorText( "Visuals & Flight" );

        ImGui::Checkbox( "Rainbow Car", &v.RainbowCar );
        ImGui::Checkbox( "Invisible Vehicle", &v.InvisibleVehicle );
        ImGui::Checkbox( "Vehicle Fly", &v.VehicleFly );
        if ( v.VehicleFly )
            ImGui::SliderFloat( "Fly Speed", &v.VehicleFlySpeed, 5.f, 100.f, "%.0f" );
    }
}
