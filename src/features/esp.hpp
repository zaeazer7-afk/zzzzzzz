#pragma once
#include <imgui.h>
#include <core/scheduler.hpp>
#include <core/config.hpp>
#include <utils/math.hpp>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <sstream>
#include <cstring>
#include <cmath>

namespace Features
{
    struct ESPPlayerData
    {
        int    ServerId;
        int    PlayerId;
        char   Name[64];
        float  Distance;
        float  Health;
        float  MaxHealth;
        float  Armor;
        bool   InVehicle;
        char   VehicleName[32];
        float  ScreenX, ScreenY;      // Head screen pos
        float  ScreenFootX, ScreenFootY;
        bool   OnScreen;
        bool   IsAlive;
        // Bone screen positions for skeleton
        float  BoneScreen[20][2];      // 20 bones, x/y each
        bool   BoneValid[20];
    };

    class ESP
    {
    public:
        static ESP& Get() { static ESP inst; return inst; }

        // Settings
        bool  Enabled       = false;
        bool  BoxESP        = true;
        bool  NameESP       = true;
        bool  HealthBar     = true;
        bool  ArmorBar      = true;
        bool  DistanceESP   = true;
        bool  Snaplines     = false;
        int   SnaplinePos   = 1;       // 0=top, 1=center, 2=bottom
        bool  Skeleton      = false;
        bool  VehicleESP    = false;
        float MaxDistance    = 500.f;
        // Colors
        ImVec4 BoxColor     = ImVec4(1.f, 1.f, 1.f, 1.f);
        ImVec4 NameColor    = ImVec4(1.f, 1.f, 1.f, 1.f);
        ImVec4 SnapColor    = ImVec4(0.3f, 1.f, 0.5f, 0.6f);
        ImVec4 SkeletonColor = ImVec4(1.f, 1.f, 1.f, 0.8f);

        void Update()
        {
            if (!Enabled) return;
            CollectPlayerData();
        }

        void Render()
        {
            if (!Enabled) return;

            auto* drawList = ImGui::GetBackgroundDrawList();
            if (!drawList) return;

            ImVec2 screen = ImGui::GetIO().DisplaySize;

            std::lock_guard<std::mutex> lk(m_dataMtx);
            for (auto& p : m_players)
            {
                if (!p.OnScreen || !p.IsAlive) continue;
                if (p.Distance > MaxDistance || p.Distance < 1.f) continue;

                float headY = p.ScreenY;
                float footY = p.ScreenFootY;
                float height = fabsf(footY - headY);
                if (height < 8.f) height = 8.f;
                float width = height * 0.45f;

                float centerX = p.ScreenX;
                float topY = headY - height * 0.1f;
                float botY = footY;

                // 2D Box
                if (BoxESP)
                {
                    // Outline
                    drawList->AddRect(
                        ImVec2(centerX - width / 2 - 1, topY - 1),
                        ImVec2(centerX + width / 2 + 1, botY + 1),
                        IM_COL32(0, 0, 0, 200), 0.f, 0, 3.f);
                    // Inner
                    drawList->AddRect(
                        ImVec2(centerX - width / 2, topY),
                        ImVec2(centerX + width / 2, botY),
                        ImGui::ColorConvertFloat4ToU32(BoxColor), 0.f, 0, 1.5f);
                }

                // Health bar (left side)
                if (HealthBar)
                {
                    float barW = 3.f;
                    float barX = centerX - width / 2 - barW - 3.f;
                    float hpFrac = (p.MaxHealth > 0) ? (p.Health / p.MaxHealth) : 0.f;
                    if (hpFrac < 0) hpFrac = 0;
                    if (hpFrac > 1) hpFrac = 1;
                    float barH = (botY - topY) * hpFrac;

                    // Background
                    drawList->AddRectFilled(
                        ImVec2(barX, topY), ImVec2(barX + barW, botY),
                        IM_COL32(0, 0, 0, 150));
                    // Health fill (green to red gradient)
                    ImU32 hpCol = IM_COL32(
                        (int)((1.f - hpFrac) * 255),
                        (int)(hpFrac * 255), 0, 255);
                    drawList->AddRectFilled(
                        ImVec2(barX, botY - barH), ImVec2(barX + barW, botY),
                        hpCol);
                }

                // Armor bar (right side)
                if (ArmorBar && p.Armor > 0)
                {
                    float barW = 3.f;
                    float barX = centerX + width / 2 + 3.f;
                    float armorFrac = p.Armor / 100.f;
                    if (armorFrac > 1) armorFrac = 1;
                    float barH = (botY - topY) * armorFrac;

                    drawList->AddRectFilled(
                        ImVec2(barX, topY), ImVec2(barX + barW, botY),
                        IM_COL32(0, 0, 0, 150));
                    drawList->AddRectFilled(
                        ImVec2(barX, botY - barH), ImVec2(barX + barW, botY),
                        IM_COL32(50, 130, 246, 255));
                }

                // Name
                if (NameESP)
                {
                    char label[128];
                    if (DistanceESP)
                        snprintf(label, sizeof(label), "%s [%dm]", p.Name, (int)p.Distance);
                    else
                        snprintf(label, sizeof(label), "%s", p.Name);

                    auto textSize = ImGui::CalcTextSize(label);
                    drawList->AddText(
                        ImVec2(centerX - textSize.x / 2, topY - textSize.y - 2),
                        ImGui::ColorConvertFloat4ToU32(NameColor), label);
                }

                // Vehicle name
                if (VehicleESP && p.InVehicle && p.VehicleName[0])
                {
                    auto vSize = ImGui::CalcTextSize(p.VehicleName);
                    drawList->AddText(
                        ImVec2(centerX - vSize.x / 2, botY + 2),
                        IM_COL32(200, 200, 100, 255), p.VehicleName);
                }

                // Snaplines
                if (Snaplines)
                {
                    ImVec2 from;
                    switch (SnaplinePos)
                    {
                    case 0: from = ImVec2(screen.x / 2, 0); break;
                    case 2: from = ImVec2(screen.x / 2, screen.y); break;
                    default: from = ImVec2(screen.x / 2, screen.y / 2); break;
                    }
                    drawList->AddLine(from,
                        ImVec2(centerX, botY),
                        ImGui::ColorConvertFloat4ToU32(SnapColor), 1.2f);
                }

                // Skeleton
                if (Skeleton)
                {
                    // Bone connections: head(0)->neck(1)->spine(2)->pelvis(3)
                    //                   neck->lshoulder(4)->lelbow(5)->lhand(6)
                    //                   neck->rshoulder(7)->relbow(8)->rhand(9)
                    //                   pelvis->lhip(10)->lknee(11)->lfoot(12)
                    //                   pelvis->rhip(13)->rknee(14)->rfoot(15)
                    const int bones[][2] = {
                        {0,1},{1,2},{2,3},
                        {1,4},{4,5},{5,6},
                        {1,7},{7,8},{8,9},
                        {3,10},{10,11},{11,12},
                        {3,13},{13,14},{14,15}
                    };

                    ImU32 skelCol = ImGui::ColorConvertFloat4ToU32(SkeletonColor);
                    for (auto& b : bones)
                    {
                        if (p.BoneValid[b[0]] && p.BoneValid[b[1]])
                        {
                            drawList->AddLine(
                                ImVec2(p.BoneScreen[b[0]][0], p.BoneScreen[b[0]][1]),
                                ImVec2(p.BoneScreen[b[1]][0], p.BoneScreen[b[1]][1]),
                                skelCol, 1.5f);
                        }
                    }
                }
            }
        }

        const std::vector<ESPPlayerData>& GetPlayers() const { return m_players; }

    private:
        ESP() = default;

        void CollectPlayerData()
        {
            static const char* collectScript = R"(
local me = PlayerId()
local myPed = PlayerPedId()
local myCoords = GetEntityCoords(myPed)
local result = ""

for _,id in ipairs(GetActivePlayers()) do
    if id ~= me then
        local ped = GetPlayerPed(id)
        if DoesEntityExist(ped) then
            local sid = GetPlayerServerId(id)
            local name = GetPlayerName(id)
            local coords = GetEntityCoords(ped)
            local health = GetEntityHealth(ped)
            local maxHp = GetEntityMaxHealth(ped)
            local armor = GetPedArmour(ped)
            local alive = not IsPedDeadOrDying(ped, true)
            local dist = #(myCoords - coords)
            local inVeh = IsPedInAnyVehicle(ped, false)
            local vehName = ""
            if inVeh then
                local veh = GetVehiclePedIsIn(ped, false)
                vehName = GetDisplayNameFromVehicleModel(GetEntityModel(veh))
            end

            -- World to screen for head and feet
            local headPos = GetPedBoneCoords(ped, 31086, 0.0, 0.0, 0.0)
            local footPos = GetEntityCoords(ped)
            local onScreen1, sx1, sy1 = GetScreenCoordFromWorldCoord(headPos.x, headPos.y, headPos.z)
            local onScreen2, sx2, sy2 = GetScreenCoordFromWorldCoord(footPos.x, footPos.y, footPos.z)

            -- Bone positions for skeleton
            local boneIds = {31086,39317,24818,11816,64729,40269,18905,45509,28252,58271,58271,36864,65245,46078,45454,52301,14201,2108,61163,23553}
            local boneStr = ""
            for bi,bid in ipairs(boneIds) do
                local bc = GetPedBoneCoords(ped, bid, 0.0, 0.0, 0.0)
                local bon, bx, by = GetScreenCoordFromWorldCoord(bc.x, bc.y, bc.z)
                boneStr = boneStr .. (bon and "1" or "0") .. "," .. string.format("%.4f,%.4f", bx, by)
                if bi < #boneIds then boneStr = boneStr .. ";" end
            end

            result = result .. string.format("%d|%d|%s|%.1f|%d|%d|%d|%s|%s|%s|%.4f|%.4f|%.4f|%.4f|%s\n",
                sid, id, name, dist, health, maxHp, armor,
                alive and "1" or "0",
                inVeh and "1" or "0", vehName,
                onScreen1 and sx1 or 0, onScreen1 and sy1 or 0,
                onScreen2 and sx2 or 0, onScreen2 and sy2 or 0,
                boneStr)
        end
    end
end
return result
)";

            auto res = Core::g_Scheduler.Execute(collectScript, "esp_collect");
            if (!res.Ok || res.Output.empty()) return;

            std::vector<ESPPlayerData> newPlayers;
            ImVec2 screen = ImGui::GetIO().DisplaySize;

            std::istringstream stream(res.Output);
            std::string line;
            while (std::getline(stream, line))
            {
                if (line.empty()) continue;
                ESPPlayerData p{};
                ParsePlayerLine(line, p, screen);
                newPlayers.push_back(p);
            }

            std::lock_guard<std::mutex> lk(m_dataMtx);
            m_players = std::move(newPlayers);
        }

        void ParsePlayerLine(const std::string& line, ESPPlayerData& p, ImVec2 screen)
        {
            std::vector<std::string> parts;
            std::stringstream ss(line);
            std::string part;
            while (std::getline(ss, part, '|'))
                parts.push_back(part);

            if (parts.size() < 15) return;

            try {
                p.ServerId  = std::stoi(parts[0]);
                p.PlayerId  = std::stoi(parts[1]);
                strncpy(p.Name, parts[2].c_str(), sizeof(p.Name) - 1);
                p.Name[sizeof(p.Name) - 1] = '\0';
                p.Distance  = std::stof(parts[3]);
                p.Health    = (float)std::stoi(parts[4]);
                p.MaxHealth = (float)std::stoi(parts[5]);
                p.Armor     = (float)std::stoi(parts[6]);
                p.IsAlive   = parts[7] == "1";
                p.InVehicle = parts[8] == "1";
                strncpy(p.VehicleName, parts[9].c_str(), sizeof(p.VehicleName) - 1);
                p.VehicleName[sizeof(p.VehicleName) - 1] = '\0';

                float sx1 = std::stof(parts[10]);
                float sy1 = std::stof(parts[11]);
                float sx2 = std::stof(parts[12]);
                float sy2 = std::stof(parts[13]);

                p.ScreenX     = sx1 * screen.x;
                p.ScreenY     = sy1 * screen.y;
                p.ScreenFootX = sx2 * screen.x;
                p.ScreenFootY = sy2 * screen.y;
                p.OnScreen    = (sx1 > 0 || sy1 > 0) && (sx2 > 0 || sy2 > 0);

                // Parse bone data
                if (parts.size() > 14)
                {
                    std::stringstream boneStream(parts[14]);
                    std::string bonePart;
                    int boneIdx = 0;
                    while (std::getline(boneStream, bonePart, ';') && boneIdx < 20)
                    {
                        std::stringstream bs(bonePart);
                        std::string bv;
                        std::vector<std::string> bvals;
                        while (std::getline(bs, bv, ',')) bvals.push_back(bv);
                        if (bvals.size() >= 3)
                        {
                            p.BoneValid[boneIdx] = bvals[0] == "1";
                            p.BoneScreen[boneIdx][0] = std::stof(bvals[1]) * screen.x;
                            p.BoneScreen[boneIdx][1] = std::stof(bvals[2]) * screen.y;
                        }
                        boneIdx++;
                    }
                }
            }
            catch (...) {}
        }

        std::mutex                   m_dataMtx;
        std::vector<ESPPlayerData>   m_players;
    };

    inline ESP& g_ESP = ESP::Get();
}
