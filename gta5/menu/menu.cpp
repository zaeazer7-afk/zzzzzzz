#include "menu.hpp"
#include "../features/self.hpp"
#include "../features/vehicle.hpp"
#include "../features/teleport.hpp"
#include "../features/weapon.hpp"
#include "../natives/invoker.hpp"
#include "../natives/hashes.hpp"
#include "../natives/types.hpp"

#include <imgui.h>
#include <Windows.h>
#include <cstring>

namespace gta5::menu {

static bool s_Open    = false;
static bool s_Inited  = false;

// Self toggles
static bool s_GodMode      = false;
static bool s_NeverWanted  = false;
static bool s_NoRagdoll    = false;
static bool s_Invisible    = false;
static int  s_WantedLevel  = 0;

// Vehicle toggles
static bool s_VehicleGod   = false;
static char s_VehicleModel[64] = "adder";

// Teleport coords
static float s_TpX = 0.0f;
static float s_TpY = 0.0f;
static float s_TpZ = 0.0f;

void Init() {
    s_Inited = true;
}

bool IsOpen() {
    return s_Open;
}

void Toggle() {
    s_Open = !s_Open;
}

void HandleInput(unsigned int msg, unsigned long long wParam, long long /*lParam*/) {
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        Toggle();
    }
}

static void TabSelf() {
    ImGui::Checkbox("God Mode", &s_GodMode);
    ImGui::Checkbox("Never Wanted", &s_NeverWanted);
    ImGui::Checkbox("No Ragdoll", &s_NoRagdoll);
    ImGui::Checkbox("Invisible", &s_Invisible);

    ImGui::Separator();

    if (ImGui::Button("Heal"))
        features::Heal();

    ImGui::SameLine();
    if (ImGui::Button("Max Armor"))
        features::SetArmor(100);

    ImGui::Separator();

    if (ImGui::SliderInt("Wanted Level", &s_WantedLevel, 0, 5))
        features::SetWantedLevel(s_WantedLevel);
}

static void TabVehicle() {
    ImGui::InputText("Model", s_VehicleModel, sizeof(s_VehicleModel));

    if (ImGui::Button("Spawn")) {
        using namespace gta5::natives;
        Hash h = NativeInvoker::Invoke<Hash>(hash::GET_HASH_KEY, reinterpret_cast<uint64_t>(s_VehicleModel));
        if (h)
            features::SpawnVehicle(h);
    }

    ImGui::Separator();

    if (ImGui::Button("Repair"))
        features::RepairVehicle();

    ImGui::SameLine();
    if (ImGui::Button("Boost"))
        features::BoostSpeed(30.0f);

    ImGui::Checkbox("Vehicle God Mode", &s_VehicleGod);
}

static void TabTeleport() {
    if (ImGui::Button("Teleport to Waypoint"))
        features::TeleportToWaypoint();

    ImGui::Separator();
    ImGui::Text("Manual Coordinates:");
    ImGui::InputFloat("X", &s_TpX);
    ImGui::InputFloat("Y", &s_TpY);
    ImGui::InputFloat("Z", &s_TpZ);

    if (ImGui::Button("Teleport"))
        features::TeleportToCoords(s_TpX, s_TpY, s_TpZ);
}

static void TabWeapon() {
    if (ImGui::Button("Give All Weapons"))
        features::GiveAllWeapons();

    ImGui::SameLine();
    if (ImGui::Button("Remove All Weapons"))
        features::RemoveAllWeapons();
}

void Render() {
    if (!s_Open || !s_Inited)
        return;

    ImGui::StyleColorsDark();

    ImGui::SetNextWindowSize(ImVec2(420, 340), ImGuiCond_FirstUseEver);
    ImGui::Begin("GTA5 Enhanced", &s_Open, ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Self")) {
            TabSelf();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Vehicle")) {
            TabVehicle();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Teleport")) {
            TabTeleport();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Weapon")) {
            TabWeapon();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

    // Run feature ticks
    features::SelfTick(s_GodMode, s_NeverWanted, s_NoRagdoll, s_Invisible);
    if (s_VehicleGod)
        features::GodModeVehicle(true);
}

} // namespace gta5::menu
