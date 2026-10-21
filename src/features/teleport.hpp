#pragma once
#include <core/scheduler.hpp>
#include <utils/math.hpp>
#include <string>
#include <vector>
#include <deque>
#include <chrono>

namespace Features
{
    struct SavedLocation
    {
        std::string name;
        float x, y, z, heading;
    };

    class Teleport
    {
    public:
        static Teleport& Get() { static Teleport inst; return inst; }

        std::vector<SavedLocation> Saved;
        std::vector<SavedLocation> Presets;
        std::deque<SavedLocation>  History;
        static constexpr int MaxHistory = 20;

        float CustomX = 0.f, CustomY = 0.f, CustomZ = 0.f;
        char  LocationNameBuf[64] = {};

        void Update()
        {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastHistoryTick).count();
            if (dt >= 5.0f)
            {
                m_lastHistoryTick = now;
                m_historyPending = true;
            }
        }

        void TeleportBack()
        {
            if (History.size() < 2) return;
            History.pop_back();
            auto& loc = History.back();
            TeleportToCoords(loc.x, loc.y, loc.z);
        }

        void RecordHistory(float x, float y, float z, float h)
        {
            if (!m_historyPending) return;
            m_historyPending = false;
            if (!History.empty())
            {
                auto& last = History.back();
                float dx = x - last.x, dy = y - last.y;
                if (dx*dx + dy*dy < 4.f) return;
            }
            History.push_back({"auto", x, y, z, h});
            while ((int)History.size() > MaxHistory) History.pop_front();
        }

        void TeleportToCoords(float x, float y, float z)
        {
            std::string code = R"(
                local ped = PlayerPedId()
                RequestCollisionAtCoord()" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + R"()
                local found, groundZ = GetGroundZFor_3dCoord()" +
                std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z + 100.f) + R"(, false)
                local finalZ = found and groundZ + 1.0 or )" + std::to_string(z) + R"(
                SetEntityCoordsNoOffset(ped, )" + std::to_string(x) + ", " + std::to_string(y) + R"(, finalZ, false, false, false)
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "tp_coords");
        }

        void TeleportToWaypoint()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local blip = GetFirstBlipInfoId(8)
                if DoesBlipExist(blip) then
                    local c = GetBlipInfoIdCoord(blip)
                    local ped = PlayerPedId()
                    local found, z = false, 0.0
                    for height = 1.0, 1000.0, 25.0 do
                        SetEntityCoordsNoOffset(ped, c.x, c.y, height, false, false, false)
                        Wait(50)
                        found, z = GetGroundZFor_3dCoord(c.x, c.y, height, false)
                        if found then break end
                    end
                    if found then
                        SetEntityCoordsNoOffset(ped, c.x, c.y, z + 1.0, false, false, false)
                    else
                        SetEntityCoordsNoOffset(ped, c.x, c.y, 200.0, false, false, false)
                    end
                end
            )", "tp_waypoint");
        }

        void TeleportForward(float dist = 10.f)
        {
            std::string code = R"(
                local ped = PlayerPedId()
                local h = GetEntityHeading(ped)
                local c = GetEntityCoords(ped)
                local rad = math.rad(h)
                local dist = )" + std::to_string(dist) + R"(
                local nx = c.x - math.sin(rad) * dist
                local ny = c.y + math.cos(rad) * dist
                SetEntityCoordsNoOffset(ped, nx, ny, c.z, false, false, false)
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "tp_forward");
        }

        void TeleportIntoClosestVehicle()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local ped = PlayerPedId()
                local c = GetEntityCoords(ped)
                local closest = nil
                local closestDist = 999999.0
                local handle, veh = FindFirstVehicle()
                local found = true
                while found do
                    local vc = GetEntityCoords(veh)
                    local d = #(c - vc)
                    if d < closestDist and not IsPedInVehicle(ped, veh, false) then
                        closest = veh
                        closestDist = d
                    end
                    found, veh = FindNextVehicle(handle)
                end
                EndFindVehicle(handle)
                if closest then
                    local seats = GetVehicleMaxNumberOfPassengers(closest)
                    for i = -1, seats - 1 do
                        if IsVehicleSeatFree(closest, i) then
                            SetPedIntoVehicle(ped, closest, i)
                            break
                        end
                    end
                end
            )", "tp_vehicle");
        }

        void SaveLocation(const char* name)
        {
            Saved.push_back({name, 0, 0, 0, 0});
            int idx = static_cast<int>(Saved.size()) - 1;
            std::string code = R"(
                local ped = PlayerPedId()
                local c = GetEntityCoords(ped)
                local h = GetEntityHeading(ped)
                return string.format("%.2f|%.2f|%.2f|%.2f", c.x, c.y, c.z, h)
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "save_loc");
        }

        void TeleportToSaved(int index)
        {
            if (index < 0 || index >= static_cast<int>(Saved.size())) return;
            auto& loc = Saved[index];
            TeleportToCoords(loc.x, loc.y, loc.z);
        }

        void TeleportToPreset(int index)
        {
            if (index < 0 || index >= static_cast<int>(Presets.size())) return;
            auto& loc = Presets[index];
            TeleportToCoords(loc.x, loc.y, loc.z);
        }

    private:
        std::chrono::steady_clock::time_point m_lastHistoryTick = std::chrono::steady_clock::now();
        bool m_historyPending = false;

        Teleport()
        {
            Presets = {
                {"LS Airport",       -1034.6f, -2733.6f, 20.2f,  0.f},
                {"Military Base",    -2047.4f,  3132.7f, 32.8f,  0.f},
                {"Mount Chiliad",      501.6f,  5604.7f, 797.9f, 0.f},
                {"Casino",             924.4f,    47.0f,  81.1f,  0.f},
                {"Del Perro Pier",   -1853.0f, -1231.5f, 13.0f,  0.f},
                {"Hospital",           295.8f, -1446.3f, 29.9f,  0.f},
                {"Police Station",     441.1f,  -981.2f, 30.7f,  0.f},
                {"Maze Bank Tower",    -75.0f,  -818.2f, 326.2f, 0.f},
                {"Fort Zancudo",     -2444.1f,  3263.3f, 32.8f,  0.f},
                {"Sandy Shores",      1848.7f,  3694.0f, 34.3f,  0.f},
                {"Paleto Bay",        -168.0f,  6325.5f, 31.6f,  0.f},
                {"LS Customs",        -337.0f,  -136.8f, 39.0f,  0.f},
                {"Simeon's",           -34.4f, -1088.2f, 26.4f,  0.f},
                {"Ammunation",         253.6f,   -50.0f, 69.9f,  0.f},
                {"Strip Club",         127.3f,  -1307.7f,29.2f,  0.f},
                {"Vinewood Sign",      711.4f,  1198.1f, 348.5f, 0.f},
            };
        }
    };

    inline Teleport& g_Teleport = Teleport::Get();
}
