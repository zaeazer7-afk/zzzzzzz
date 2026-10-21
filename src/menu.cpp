#include "menu.h"
#include "game.h"
#include "render.h"
#include "memory.h"

#include <imgui.h>
#include <cmath>

namespace {
    struct {
        bool godMode          = false;
        bool neverWanted      = false;
        bool superJump        = false;
        bool fastRun          = false;
        bool noRagdoll        = false;
        bool invisible        = false;
        bool noClip           = false;
        bool infiniteAmmo     = false;
        bool infiniteClip     = false;
        bool explosiveAmmo    = false;
        bool fireAmmo         = false;
        bool explosiveMelee   = false;
        bool vehGodMode       = false;
        bool seatbelt         = false;
        bool hornBoost        = false;
        bool freezeTime       = false;
        bool nightVision      = false;
        bool thermalVision    = false;

        f32  runSpeed         = 1.0f;
        f32  swimSpeed        = 1.0f;
        f32  noClipSpeed      = 2.0f;
        i32  hour             = 12;
        i32  minute           = 0;
        i32  weatherIdx       = 0;

        char vehSpawnName[64] = "adder";
        char tpX[16]          = "0";
        char tpY[16]          = "0";
        char tpZ[16]          = "0";

        i32  charSlot         = 0;
        i32  moneyAmount      = 1000000;
        i32  targetRank       = 120;
        f32  targetKD         = 2.5f;
        i32  targetKills      = 5000;
    } s;

    const char* weatherNames[] = {
        "EXTRASUNNY", "CLEAR", "CLOUDS", "SMOG", "FOGGY",
        "OVERCAST", "RAIN", "THUNDER", "CLEARING", "NEUTRAL",
        "SNOW", "BLIZZARD", "SNOWLIGHT", "XMAS", "HALLOWEEN"
    };
    constexpr int weatherCount = sizeof(weatherNames) / sizeof(weatherNames[0]);

    bool requestModel(Hash hash) {
        game::invoke<void>(nh::REQUEST_MODEL, hash);
        for (int i = 0; i < 100; i++) {
            if (game::invoke<BOOL>(nh::HAS_MODEL_LOADED, hash)) return true;
            Sleep(10);
        }
        return false;
    }

    void tabSelf() {
        ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.f, 1.f), "Self");
        ImGui::Separator();
        ImGui::Checkbox("God Mode",       &s.godMode);
        ImGui::Checkbox("Never Wanted",   &s.neverWanted);
        ImGui::Checkbox("Super Jump",     &s.superJump);
        ImGui::Checkbox("No Ragdoll",     &s.noRagdoll);
        ImGui::Checkbox("Invisible",      &s.invisible);
        ImGui::Checkbox("No Clip",        &s.noClip);
        ImGui::Checkbox("Fast Run",       &s.fastRun);
        if (s.fastRun) ImGui::SliderFloat("Run Speed", &s.runSpeed, 1.0f, 10.0f);
        ImGui::SliderFloat("Swim Speed", &s.swimSpeed, 1.0f, 10.0f);
        if (s.noClip) ImGui::SliderFloat("No Clip Speed", &s.noClipSpeed, 0.5f, 20.0f);
        ImGui::Spacing();
        if (ImGui::Button("Heal", ImVec2(120, 0))) {
            auto ped = game::localPed();
            if (ped) {
                mem::write<f32>(ped + off::Ped_Health, mem::read<f32>(ped + off::Ped_MaxHealth));
                mem::write<f32>(ped + off::Ped_Armor, 50.f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Max Armor", ImVec2(120, 0))) {
            auto ped = game::localPed();
            if (ped) mem::write<f32>(ped + off::Ped_Armor, 100.f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Fill Ability", ImVec2(120, 0))) {
            auto pid = game::invoke<Player>(nh::PLAYER_ID);
            game::invoke<void>(nh::SPECIAL_ABILITY_FILL_METER, pid, 1);
        }
    }

    void tabWeapons() {
        ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.f, 1.f), "Weapons");
        ImGui::Separator();
        ImGui::Checkbox("Infinite Ammo",     &s.infiniteAmmo);
        ImGui::Checkbox("Infinite Clip",     &s.infiniteClip);
        ImGui::Checkbox("Explosive Ammo",    &s.explosiveAmmo);
        ImGui::Checkbox("Fire Ammo",         &s.fireAmmo);
        ImGui::Checkbox("Explosive Melee",   &s.explosiveMelee);
        ImGui::Spacing();
        if (ImGui::Button("Give All Weapons", ImVec2(200, 0))) {
            auto ped = game::invoke<Ped>(nh::PLAYER_PED_ID);
            Hash weapons[] = {
                0x1B06D571, 0x5EF9FEC4, 0x22D8FE39, 0x3656C8C1, 0x99AEEB3B,
                0xBFD21232, 0x88374054, 0xD205520E, 0x83839C4,  0x47757124,
                0xC1B3C3D1, 0x7846A318, 0xE284C527, 0x9D61E50F, 0xA3D4D34,
                0xBFEFFF6D, 0x394F415C, 0x83BF0278, 0xFAD1F1C9, 0xAF113F99,
                0xC0A3098D, 0x969C3D67, 0x7F229F94, 0x84D6FAFD, 0x624FE830,
                0x9D07F764, 0x7FD62962, 0xDBBD7280, 0x61012683, 0xC734385A,
                0xA284510B, 0x4DD2DC56, 0xB1CA77B1, 0x687652CE, 0x42BF8A85,
                0x93E220BD, 0x2BE6766B, 0x78A97CD0, 0xEFE7E2DF, 0x0A3D4D34,
                0xCD274149, 0x171789FC
            };
            for (auto h : weapons)
                game::invoke<void>(nh::GIVE_WEAPON_TO_PED, ped, h, 9999, false, true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove All", ImVec2(200, 0))) {
            auto ped = game::invoke<Ped>(nh::PLAYER_PED_ID);
            game::invoke<void>(nh::REMOVE_ALL_PED_WEAPONS, ped, true);
        }
    }

    void tabVehicle() {
        ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.f, 1.f), "Vehicle");
        ImGui::Separator();
        ImGui::Checkbox("Vehicle God Mode", &s.vehGodMode);
        ImGui::Checkbox("Seatbelt",         &s.seatbelt);
        ImGui::Checkbox("Horn Boost",       &s.hornBoost);
        ImGui::Spacing();
        if (ImGui::Button("Repair Vehicle", ImVec2(200, 0))) {
            auto veh = game::localVehicle();
            if (veh) {
                mem::write<f32>(veh + off::Veh_Health,    1000.f);
                mem::write<f32>(veh + off::Veh_Body,      1000.f);
                mem::write<f32>(veh + off::Veh_Engine,    1000.f);
                mem::write<f32>(veh + off::Veh_Petrol,    1000.f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Boost Forward", ImVec2(200, 0))) {
            auto pedId = game::invoke<Ped>(nh::PLAYER_PED_ID);
            auto veh = game::localVehicle();
            if (veh) game::invoke<void>(nh::SET_VEHICLE_FORWARD_SPEED, pedId, 50.f);
        }
        ImGui::Spacing();
        ImGui::InputText("Model Name", s.vehSpawnName, sizeof(s.vehSpawnName));
        if (ImGui::Button("Spawn Vehicle", ImVec2(200, 0))) {
            auto pid = game::invoke<Ped>(nh::PLAYER_PED_ID);
            Hash model = game::invoke<Hash>(nh::GET_HASH_KEY, (u64)s.vehSpawnName);
            if (requestModel(model)) {
                auto pos = game::localPos();
                auto heading = game::invoke<f32>(nh::GET_ENTITY_HEADING, pid);
                Vehicle veh = game::invoke<Vehicle>(nh::CREATE_VEHICLE,
                    model, pos.x, pos.y, pos.z + 1.f, heading, true, false, false);
                game::invoke<void>(nh::SET_PED_INTO_VEHICLE, pid, veh, (i32)-1);
                game::invoke<void>(nh::SET_VEHICLE_ENGINE_ON, veh, true, true, false);
                game::invoke<void>(nh::SET_MODEL_AS_NO_LONGER_NEEDED, model);
                game::invoke<void>(nh::SET_ENTITY_NO_LONGER_NEEDED, (u64)&veh);
            }
        }
    }

    void tabTeleport() {
        ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.f, 1.f), "Teleport");
        ImGui::Separator();
        if (ImGui::Button("Teleport to Waypoint", ImVec2(250, 0))) {
            if (game::invoke<BOOL>(nh::IS_WAYPOINT_ACTIVE)) {
                Blip blip = game::invoke<Blip>(nh::GET_FIRST_BLIP_INFO_ID, (i32)8);
                if (blip) {
                    NativeVector3 coords = game::invoke<NativeVector3>(nh::GET_BLIP_INFO_ID_COORD, blip);
                    f32 groundZ = 0.f;
                    bool found = false;
                    for (f32 z = 800.f; z >= 0.f && !found; z -= 25.f) {
                        auto ped = game::invoke<Ped>(nh::PLAYER_PED_ID);
                        game::invoke<void>(nh::SET_ENTITY_COORDS, ped,
                            coords.x, coords.y, z, false, false, false, false);
                        Sleep(50);
                        found = game::invoke<BOOL>(nh::GET_GROUND_Z_FOR_3D_COORD,
                            coords.x, coords.y, z, (u64)&groundZ, false, false);
                    }
                    auto ped = game::invoke<Ped>(nh::PLAYER_PED_ID);
                    game::invoke<void>(nh::SET_ENTITY_COORDS, ped,
                        coords.x, coords.y, found ? groundZ + 1.f : 300.f,
                        false, false, false, false);
                }
            }
        }
        ImGui::Spacing();
        auto pos = game::localPos();
        ImGui::Text("Current: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
        ImGui::InputText("X", s.tpX, sizeof(s.tpX));
        ImGui::InputText("Y", s.tpY, sizeof(s.tpY));
        ImGui::InputText("Z", s.tpZ, sizeof(s.tpZ));
        if (ImGui::Button("Teleport to Coords", ImVec2(250, 0))) {
            auto ped = game::invoke<Ped>(nh::PLAYER_PED_ID);
            game::invoke<void>(nh::SET_ENTITY_COORDS, ped,
                (f32)atof(s.tpX), (f32)atof(s.tpY), (f32)atof(s.tpZ),
                false, false, false, false);
        }
        ImGui::Spacing();
        ImGui::Text("Quick Teleport:");
        struct Loc { const char* name; f32 x, y, z; };
        Loc locs[] = {
            {"Airport",        -1336.f, -3044.f, 13.9f},
            {"Military Base",  -2047.f,  3132.f, 32.8f},
            {"Mt. Chiliad",      501.f,  5604.f, 797.9f},
            {"Maze Bank Tower",  -75.f,  -818.f, 326.2f},
            {"Pier",           -1850.f, -1231.f, 13.0f},
        };
        for (auto& l : locs) {
            if (ImGui::Button(l.name, ImVec2(200, 0))) {
                auto ped = game::invoke<Ped>(nh::PLAYER_PED_ID);
                game::invoke<void>(nh::SET_ENTITY_COORDS, ped, l.x, l.y, l.z, false, false, false, false);
            }
        }
    }

    void tabWorld() {
        ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.f, 1.f), "World");
        ImGui::Separator();
        ImGui::Checkbox("Freeze Time",    &s.freezeTime);
        ImGui::Checkbox("Night Vision",   &s.nightVision);
        ImGui::Checkbox("Thermal Vision", &s.thermalVision);
        ImGui::Spacing();
        ImGui::SliderInt("Hour",   &s.hour,   0, 23);
        ImGui::SliderInt("Minute", &s.minute,  0, 59);
        if (ImGui::Button("Set Time", ImVec2(150, 0)))
            game::invoke<void>(nh::NETWORK_OVERRIDE_CLOCK_TIME, s.hour, s.minute, 0);
        ImGui::Spacing();
        ImGui::Combo("Weather", &s.weatherIdx, weatherNames, weatherCount);
        if (ImGui::Button("Set Weather", ImVec2(150, 0)))
            game::invoke<void>(nh::SET_WEATHER_TYPE_NOW_PERSIST, (u64)weatherNames[s.weatherIdx]);
        ImGui::SameLine();
        if (ImGui::Button("Clear Override", ImVec2(150, 0)))
            game::invoke<void>(nh::CLEAR_WEATHER_TYPE_PERSIST);
    }

    Hash statHash(const char* statName) {
        char full[128];
        snprintf(full, sizeof(full), "MP%d_%s", s.charSlot, statName);
        return game::invoke<Hash>(nh::GET_HASH_KEY, (u64)full);
    }

    void setStat(const char* statName, i32 value) {
        game::invoke<void>(nh::STAT_SET_INT, statHash(statName), value, true);
    }

    void setStatBool(const char* statName, bool value) {
        game::invoke<void>(nh::STAT_SET_BOOL, statHash(statName), value ? 1 : 0, true);
    }

    void tabRecovery() {
        ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.f, 1.f), "Recovery");
        ImGui::Separator();
        ImGui::Combo("Character Slot", &s.charSlot, "MP0 (Char 1)\0MP1 (Char 2)\0");
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.10f, 0.70f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.55f, 0.18f, 0.85f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.65f, 0.25f, 0.95f, 1.00f));
        if (ImGui::Button(">>> FULL RECOVERY <<<", ImVec2(-1, 40))) {
            {
                i32 rp = 120 * 120 * 50 + 120 * 100;
                setStat("CHAR_SET_RP_GIFT_ADMIN", rp);
                setStat("CHAR_XP_FM", rp);
                setStat("CREW_LOCAL_XP_0", 50000000);
            }
            setStat("BANK_BALANCE",   2000000000);
            setStat("WALLET_BALANCE", 2000000000);
            setStat("CASINO_CHIPS",   2000000000);
            {
                const char* ws[] = {
                    "CHAR_FM_WEAP_UNLOCKED", "CHAR_FM_WEAP_UNLOCKED2",
                    "CHAR_FM_WEAP_UNLOCKED3", "CHAR_FM_WEAP_UNLOCKED4"
                };
                for (auto& w : ws) setStat(w, -1);
                for (int i = 0; i < 80; i++) {
                    char buf[64];
                    for (int a = 1; a <= 4; a++) {
                        snprintf(buf, sizeof(buf), "CHAR_FM_WEAP_ADDON_%d_UNLCK%d", a, i);
                        setStat(buf, -1);
                    }
                }
            }
            for (int i = 0; i < 300; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "DLC_APPAREL_ACQUIRED_%d", i);
                setStatBool(buf, true);
            }
            for (int i = 0; i < 500; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "CLTH_ACQUIRED_%d", i);
                setStat(buf, -1);
            }
            for (int i = 0; i < 50; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "TATTOO_FM_UNLOCKS_%d", i);
                setStat(buf, -1);
                snprintf(buf, sizeof(buf), "TATTOO_FM_CURRENT_%d", i);
                setStat(buf, -1);
            }
            for (int i = 0; i < 100; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "HAIR_COLOUR_PURCHASED_%d", i);
                setStatBool(buf, true);
            }
            for (int i = 0; i < 50; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "ADMIN_CLOTHES_GV_BS_%d", i);
                setStat(buf, -1);
            }
            for (int i = 0; i < 20; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "CHAR_FM_CARMOD_1_UNLCK%d", i);
                setStat(buf, -1);
            }
            {
                const char* tp[] = {
                    "YOURGANGHP_TUT_DONE", "YOURGANGHP_FLOWTYPE",
                    "YOURGANGHP_YOURRANK", "DLC_VEHICL_DVTRADE"
                };
                for (auto& t : tp) setStat(t, -1);
                for (int i = 0; i < 50; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "YOURGANGHP_YOURTP_%d", i);
                    setStat(buf, -1);
                }
                for (int i = 0; i < 100; i++) {
                    char buf[64];
                    for (int a = 1; a <= 3; a++) {
                        snprintf(buf, sizeof(buf), "CHAR_FM_CARMOD_%d_UNLCK%d", a, i);
                        setStat(buf, -1);
                    }
                }
                for (int i = 0; i < 50; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "BENNY_UNLCK%d", i);
                    setStat(buf, -1);
                }
            }
            setStat("SCRIPT_INCREASE_STAM", 100);
            setStat("SCRIPT_INCREASE_STRN", 100);
            setStat("SCRIPT_INCREASE_LUNG", 100);
            setStat("SCRIPT_INCREASE_DRIV", 100);
            setStat("SCRIPT_INCREASE_FLY",  100);
            setStat("SCRIPT_INCREASE_SHO",  100);
            setStat("SCRIPT_INCREASE_STL",  100);
            setStat("KILLS_PLAYERS", 5000);
            setStat("DEATHS_PLAYER", 2000);
            setStat("HEIST_PLANNING_BOARD_COMPLETED", 127);
            for (int i = 0; i < 6; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "CR_HEIST_%d_DONE", i);
                setStat(buf, 100);
            }
            setStat("YOURGANGHP_TUT_DONE", 1);
            setStat("CAS_HEIST_FLOW",     16383);
            setStat("H3_COMPLETEDPOSIX",   -1);
            setStat("H3OPT_BITSET1",       -1);
            setStat("H3OPT_BITSET0",       -1);
            setStat("H4_PROGRESS",         131071);
            setStat("H4CNF_BS_GEN",        131071);
            setStat("H4CNF_APPROACH",      -1);
            setStat("CLUB_POPULARITY", 1000);
            setStat("INTOTALRESUP",  500);
            setStat("INTOTALRESUP2", 500);
            for (int i = 0; i < 52; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "RESEARCH_PROGRESS_%d", i);
                setStat(buf, 100);
                snprintf(buf, sizeof(buf), "RESEARCH_COST_%d", i);
                setStat(buf, 0);
            }
            setStat("NGR_MFG_BS_TOTAL",    500);
            for (int i = 0; i < 20; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "FM_ACT_PH%d", i);
                setStat(buf, -1);
            }
            setStat("FM_ACT_PHN",    -1);
            setStat("DVMIS_DONE_BS", -1);
            for (int i = 0; i < 100; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "AWARD_STATUS_%d", i);
                setStat(buf, -1);
                snprintf(buf, sizeof(buf), "DVADV_FM_CS_%d", i);
                setStat(buf, -1);
            }
            {
                const char* cols[] = {
                    "PSYCHE_COLLECTED",  "PSYCHE2_COLLECTED",
                    "PSYCHE3_COLLECTED", "PSYCHE4_COLLECTED",
                    "PSYCHE5_COLLECTED", "PSYCHE6_COLLECTED",
                    "PSYCHE7_COLLECTED", "PSYCHE8_COLLECTED",
                    "PSYCHE9_COLLECTED", "PSYCHE10_COLLECTED",
                    "MOVIE_PROPS_COLLECTED", "SIGNAL_JAMMER_COLLECTED",
                    "LD_CARDS_COLLECTED", "FIGURES_COLLECTED",
                    "RADIOACTIVITY_COLLECTED", "SNOWFLAKE_COLLECTED",
                    "BURIED_STASH_COLLECTED"
                };
                for (auto& c : cols) setStat(c, -1);
            }
            for (int i = 0; i < 80; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "DVACH_%d", i);
                setStat(buf, -1);
            }
            setStat("HSW_TR_PRC_BOUGHT_0", -1);
            setStat("HSW_TR_PRC_BOUGHT_1", -1);
            setStat("HSW_TR_PRC_BOUGHT_2", -1);
            for (int i = 0; i < 30; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "HSW_UNLCK%d", i);
                setStat(buf, -1);
            }
            setStat("HSW_RACES_WON", 50);
            setStat("FIXER_GENERAL_BS", -1);
            setStat("FIXER_STORY_COMPLETE", 1);
            setStat("FIXER_COMPLETED_MISSIONS", 200);
            setStat("FIXER_SECURITY_CONTRACTS_DONE", 200);
            setStat("FIXER_PAY_PHONE_HITS_DONE", 200);
            for (int i = 0; i < 10; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "FIXER_STORY_STRAND_%d", i);
                setStat(buf, 100);
            }
            setStat("XM22_GEN_BS", -1);
            setStat("XM22_LAB_TOTAL_PRODUCT", 999);
            setStat("XM22_LAB_UPGRD_EQUIP", 1);
            setStat("XM22_LAB_UPGRD_STAFF", 1);
            setStat("XM22_LAB_TOTAL_SOLD", 500);
            setStat("SV_YARD_GENERAL_BS", -1);
            setStat("SV_YARD_PROGRESS", 100);
            setStat("SV_YARD_TOTAL_ROBBERIES", 100);
            setStat("SV_YARD_TOTAL_VEHICLES_SOLD", 100);
            setStat("YOURGANGHP_GENERAL_BS", -1);
            setStat("YOURGANGHP_PROGRESS", 100);
            setStat("YOURGANGHP_COMPLETION", -1);
            for (int i = 0; i < 100; i++) {
                char buf[64];
                snprintf(buf, sizeof(buf), "DLC_VEHICL_DVTRADE_%d", i);
                setStat(buf, -1);
            }
            setStat("DVEH_DVTRADE", -1);
        }
        ImGui::PopStyleColor(3);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Money", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputInt("Amount", &s.moneyAmount, 1000000, 10000000);
            if (s.moneyAmount < 0) s.moneyAmount = 0;
            if (s.moneyAmount > 2000000000) s.moneyAmount = 2000000000;

            if (ImGui::Button("+1M##money", ImVec2(80, 0)))   s.moneyAmount = 1000000;
            ImGui::SameLine();
            if (ImGui::Button("+10M##money", ImVec2(80, 0)))  s.moneyAmount = 10000000;
            ImGui::SameLine();
            if (ImGui::Button("+100M##money", ImVec2(80, 0))) s.moneyAmount = 100000000;
            ImGui::SameLine();
            if (ImGui::Button("Max##money", ImVec2(80, 0)))   s.moneyAmount = 2000000000;

            if (ImGui::Button("Set Wallet", ImVec2(150, 0)))
                setStat("WALLET_BALANCE", s.moneyAmount);
            ImGui::SameLine();
            if (ImGui::Button("Set Bank", ImVec2(150, 0)))
                setStat("BANK_BALANCE", s.moneyAmount);
            if (ImGui::Button("Set Casino Chips", ImVec2(150, 0)))
                setStat("CASINO_CHIPS", s.moneyAmount);
        }

        if (ImGui::CollapsingHeader("Rank / RP")) {
            ImGui::SliderInt("Target Rank", &s.targetRank, 1, 8000);
            if (ImGui::Button("Set Rank", ImVec2(200, 0))) {
                i32 rp;
                if (s.targetRank <= 100)
                    rp = s.targetRank * s.targetRank * 50 + s.targetRank * 100;
                else if (s.targetRank <= 200)
                    rp = 550000 + (s.targetRank - 100) * 50000;
                else
                    rp = 5550000 + (s.targetRank - 200) * 50000;
                if (rp < 0) rp = 2000000000;
                setStat("CHAR_SET_RP_GIFT_ADMIN", rp);
                setStat("CHAR_XP_FM", rp);
            }
            if (ImGui::Button("Set Crew Rank 100", ImVec2(200, 0)))
                setStat("CREW_LOCAL_XP_0", 50000000);
        }

        if (ImGui::CollapsingHeader("Unlock All")) {
            if (ImGui::Button("Unlock All Weapons", ImVec2(250, 0))) {
                const char* ws[] = {
                    "CHAR_FM_WEAP_UNLOCKED", "CHAR_FM_WEAP_UNLOCKED2",
                    "CHAR_FM_WEAP_UNLOCKED3", "CHAR_FM_WEAP_UNLOCKED4"
                };
                for (auto& w : ws) setStat(w, -1);
            }
            if (ImGui::Button("Unlock All Weapon Tints", ImVec2(250, 0))) {
                for (int i = 0; i < 80; i++) {
                    char buf[64];
                    for (int a = 1; a <= 4; a++) {
                        snprintf(buf, sizeof(buf), "CHAR_FM_WEAP_ADDON_%d_UNLCK%d", a, i);
                        setStat(buf, -1);
                    }
                }
            }
            if (ImGui::Button("Unlock All Clothing", ImVec2(250, 0))) {
                for (int i = 0; i < 300; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "DLC_APPAREL_ACQUIRED_%d", i);
                    setStatBool(buf, true);
                }
                for (int i = 0; i < 500; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "CLTH_ACQUIRED_%d", i);
                    setStat(buf, -1);
                }
            }
            if (ImGui::Button("Unlock All Tattoos", ImVec2(250, 0))) {
                for (int i = 0; i < 50; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "TATTOO_FM_UNLOCKS_%d", i);
                    setStat(buf, -1);
                    snprintf(buf, sizeof(buf), "TATTOO_FM_CURRENT_%d", i);
                    setStat(buf, -1);
                }
            }
            if (ImGui::Button("Unlock All Hairstyles", ImVec2(250, 0))) {
                for (int i = 0; i < 100; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "HAIR_COLOUR_PURCHASED_%d", i);
                    setStatBool(buf, true);
                }
            }
            if (ImGui::Button("Unlock All Masks", ImVec2(250, 0))) {
                for (int i = 0; i < 50; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "ADMIN_CLOTHES_GV_BS_%d", i);
                    setStat(buf, -1);
                }
            }
            if (ImGui::Button("Unlock All Parachutes", ImVec2(250, 0))) {
                for (int i = 0; i < 20; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "CHAR_FM_CARMOD_1_UNLCK%d", i);
                    setStat(buf, -1);
                }
            }
        }

        if (ImGui::CollapsingHeader("Stats")) {
            if (ImGui::Button("Max All Skills", ImVec2(250, 0))) {
                setStat("SCRIPT_INCREASE_STAM", 100);
                setStat("SCRIPT_INCREASE_STRN", 100);
                setStat("SCRIPT_INCREASE_LUNG", 100);
                setStat("SCRIPT_INCREASE_DRIV", 100);
                setStat("SCRIPT_INCREASE_FLY",  100);
                setStat("SCRIPT_INCREASE_SHO",  100);
                setStat("SCRIPT_INCREASE_STL",  100);
            }
            ImGui::Spacing();
            ImGui::SliderFloat("K/D Ratio", &s.targetKD, 0.01f, 100.0f, "%.2f");
            ImGui::InputInt("Kills", &s.targetKills, 100, 1000);
            if (s.targetKills < 0) s.targetKills = 0;
            if (ImGui::Button("Set K/D", ImVec2(150, 0))) {
                i32 deaths = (s.targetKills > 0) ? (i32)(s.targetKills / s.targetKD) : 1;
                if (deaths < 1) deaths = 1;
                setStat("KILLS_PLAYERS", s.targetKills);
                setStat("DEATHS_PLAYER", deaths);
            }
            if (ImGui::Button("Max Heist Stats", ImVec2(250, 0))) {
                setStat("HEIST_PLANNING_BOARD_COMPLETED", 127);
                for (int i = 0; i < 6; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "CR_HEIST_%d_DONE", i);
                    setStat(buf, 100);
                }
                setStat("YOURGANGHP_TUT_DONE", 1);
                setStat("CAS_HEIST_FLOW",     16383);
                setStat("H3_COMPLETEDPOSIX",   -1);
                setStat("H3OPT_BITSET1",       -1);
                setStat("H3OPT_BITSET0",       -1);
                setStat("H4_PROGRESS",         131071);
                setStat("H4CNF_BS_GEN",        131071);
                setStat("H4CNF_APPROACH",      -1);
            }
        }

        if (ImGui::CollapsingHeader("Vehicles")) {
            if (ImGui::Button("Unlock All Trade Prices", ImVec2(250, 0))) {
                const char* tp[] = {
                    "YOURGANGHP_TUT_DONE", "YOURGANGHP_FLOWTYPE",
                    "YOURGANGHP_YOURRANK", "DLC_VEHICL_DVTRADE"
                };
                for (auto& t : tp) setStat(t, -1);
                for (int i = 0; i < 50; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "YOURGANGHP_YOURTP_%d", i);
                    setStat(buf, -1);
                }
            }
            if (ImGui::Button("Unlock All LSC Upgrades", ImVec2(250, 0))) {
                for (int i = 0; i < 100; i++) {
                    char buf[64];
                    for (int a = 1; a <= 3; a++) {
                        snprintf(buf, sizeof(buf), "CHAR_FM_CARMOD_%d_UNLCK%d", a, i);
                        setStat(buf, -1);
                    }
                }
            }
            if (ImGui::Button("Unlock Benny's Upgrades", ImVec2(250, 0))) {
                for (int i = 0; i < 50; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "BENNY_UNLCK%d", i);
                    setStat(buf, -1);
                }
            }
        }

        if (ImGui::CollapsingHeader("Properties / Businesses")) {
            if (ImGui::Button("Max Nightclub Popularity", ImVec2(250, 0)))
                setStat("CLUB_POPULARITY", 1000);
            if (ImGui::Button("Complete Bunker Research", ImVec2(250, 0))) {
                setStat("INTOTALRESUP", 500);
                setStat("INTOTALRESUP2", 500);
                for (int i = 0; i < 52; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "RESEARCH_PROGRESS_%d", i);
                    setStat(buf, 100);
                    snprintf(buf, sizeof(buf), "RESEARCH_COST_%d", i);
                    setStat(buf, 0);
                }
                setStat("NGR_MFG_BS_TOTAL",    500);
                setStat("YOURGANGHP_YOURRANK", 100);
            }
            if (ImGui::Button("Max Contact Missions", ImVec2(250, 0))) {
                for (int i = 0; i < 20; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "FM_ACT_PH%d", i);
                    setStat(buf, -1);
                }
                setStat("FM_ACT_PHN",    -1);
                setStat("DVMIS_DONE_BS", -1);
            }
        }

        if (ImGui::CollapsingHeader("Enhanced / E&E")) {
            if (ImGui::Button("Unlock HSW Trade Prices", ImVec2(250, 0))) {
                setStat("HSW_TR_PRC_BOUGHT_0", -1);
                setStat("HSW_TR_PRC_BOUGHT_1", -1);
                setStat("HSW_TR_PRC_BOUGHT_2", -1);
                for (int i = 0; i < 30; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "HSW_UNLCK%d", i);
                    setStat(buf, -1);
                }
                setStat("HSW_RACES_WON", 50);
            }
            if (ImGui::Button("Complete Agency", ImVec2(250, 0))) {
                setStat("FIXER_GENERAL_BS", -1);
                setStat("FIXER_STORY_COMPLETE", 1);
                setStat("FIXER_COMPLETED_MISSIONS", 200);
                setStat("FIXER_SECURITY_CONTRACTS_DONE", 200);
                setStat("FIXER_PAY_PHONE_HITS_DONE", 200);
                for (int i = 0; i < 10; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "FIXER_STORY_STRAND_%d", i);
                    setStat(buf, 100);
                }
            }
            if (ImGui::Button("Complete Acid Lab", ImVec2(250, 0))) {
                setStat("XM22_GEN_BS", -1);
                setStat("XM22_LAB_TOTAL_PRODUCT", 999);
                setStat("XM22_LAB_UPGRD_EQUIP", 1);
                setStat("XM22_LAB_UPGRD_STAFF", 1);
                setStat("XM22_LAB_TOTAL_SOLD", 500);
            }
            if (ImGui::Button("Complete Salvage Yard", ImVec2(250, 0))) {
                setStat("SV_YARD_GENERAL_BS", -1);
                setStat("SV_YARD_PROGRESS", 100);
                setStat("SV_YARD_TOTAL_ROBBERIES", 100);
                setStat("SV_YARD_TOTAL_VEHICLES_SOLD", 100);
            }
            if (ImGui::Button("Complete Cluckin' Bell Raid", ImVec2(250, 0))) {
                setStat("YOURGANGHP_GENERAL_BS", -1);
                setStat("YOURGANGHP_PROGRESS", 100);
                setStat("YOURGANGHP_COMPLETION", -1);
                setStat("YOURGANGHP_TUT_DONE", 1);
            }
            if (ImGui::Button("Unlock All E&E Vehicles", ImVec2(250, 0))) {
                for (int i = 0; i < 100; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "DLC_VEHICL_DVTRADE_%d", i);
                    setStat(buf, -1);
                }
                setStat("DVEH_DVTRADE", -1);
            }
        }

        if (ImGui::CollapsingHeader("Misc Unlocks")) {
            if (ImGui::Button("Unlock All Awards", ImVec2(250, 0))) {
                for (int i = 0; i < 100; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "AWARD_STATUS_%d", i);
                    setStat(buf, -1);
                    snprintf(buf, sizeof(buf), "DVADV_FM_CS_%d", i);
                    setStat(buf, -1);
                }
            }
            if (ImGui::Button("Unlock All Collectibles", ImVec2(250, 0))) {
                const char* cols[] = {
                    "PSYCHE_COLLECTED",  "PSYCHE2_COLLECTED",
                    "PSYCHE3_COLLECTED", "PSYCHE4_COLLECTED",
                    "PSYCHE5_COLLECTED", "PSYCHE6_COLLECTED",
                    "PSYCHE7_COLLECTED", "PSYCHE8_COLLECTED",
                    "PSYCHE9_COLLECTED", "PSYCHE10_COLLECTED",
                    "MOVIE_PROPS_COLLECTED", "SIGNAL_JAMMER_COLLECTED",
                    "LD_CARDS_COLLECTED", "FIGURES_COLLECTED",
                    "RADIOACTIVITY_COLLECTED", "SNOWFLAKE_COLLECTED",
                    "BURIED_STASH_COLLECTED"
                };
                for (auto& c : cols) setStat(c, -1);
            }
            if (ImGui::Button("Unlock All Achievements", ImVec2(250, 0))) {
                for (int i = 0; i < 80; i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "DVACH_%d", i);
                    setStat(buf, -1);
                }
            }
        }
    }

    void tabMisc() {
        ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.f, 1.f), "Misc");
        ImGui::Separator();
        auto pos = game::localPos();
        ImGui::Text("Position: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
        auto ped = game::localPed();
        if (ped) {
            ImGui::Text("Health: %.0f / %.0f",
                mem::read<f32>(ped + off::Ped_Health),
                mem::read<f32>(ped + off::Ped_MaxHealth));
            ImGui::Text("Armor:  %.0f", mem::read<f32>(ped + off::Ped_Armor));
        }
        auto veh = game::localVehicle();
        if (veh) {
            ImGui::Text("Vehicle HP: %.0f  Engine: %.0f",
                mem::read<f32>(veh + off::Veh_Health),
                mem::read<f32>(veh + off::Veh_Engine));
        }
    }
}

void menu::draw() {
    ImGui::SetNextWindowSize(ImVec2(520, 480), ImGuiCond_FirstUseEver);
    ImGui::Begin("Enhanced | GTA V", nullptr, ImGuiWindowFlags_NoCollapse);
    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("Self"))      { tabSelf();      ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Weapons"))   { tabWeapons();   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Vehicle"))   { tabVehicle();   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Teleport"))  { tabTeleport();  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("World"))     { tabWorld();     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Recovery"))  { tabRecovery();  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Misc"))      { tabMisc();      ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void menu::tick() {
    auto ped = game::localPed();
    if (!ped) return;
    auto pid   = game::invoke<Player>(nh::PLAYER_ID);
    auto pedId = game::invoke<Ped>(nh::PLAYER_PED_ID);

    if (s.godMode) {
        f32 maxHp = mem::read<f32>(ped + off::Ped_MaxHealth);
        if (maxHp < 100.f) maxHp = 328.f;
        mem::write<f32>(ped + off::Ped_Health, maxHp);
        mem::write<f32>(ped + off::Ped_Armor, 50.f);
    }
    if (s.neverWanted) {
        auto pi = game::localPlayerInfo();
        if (pi) {
            mem::write<u32>(pi + off::PI_WantedLevel, 0);
            mem::write<u32>(pi + off::PI_WantedMax, 0);
        }
        game::invoke<void>(nh::SET_POLICE_IGNORE_PLAYER, pid, true);
        game::invoke<void>(nh::SET_EVERYONE_IGNORE_PLAYER, pid, true);
    }
    if (s.superJump)
        game::invoke<void>(nh::SET_SUPER_JUMP_THIS_FRAME, pid);
    if (s.fastRun)
        game::invoke<void>(nh::SET_RUN_SPRINT_MULTIPLIER, pid, s.runSpeed);
    if (s.swimSpeed > 1.01f)
        game::invoke<void>(nh::SET_SWIM_MULTIPLIER, pid, s.swimSpeed);
    if (s.noRagdoll) {
        game::invoke<void>(nh::SET_PED_CAN_RAGDOLL, pedId, false);
        game::invoke<void>(nh::SET_PED_CONFIG_FLAG, pedId, (i32)26, true);
    }
    if (s.invisible)
        game::invoke<void>(nh::SET_ENTITY_VISIBLE, pedId, false, false);

    if (s.noClip) {
        game::invoke<void>(nh::FREEZE_ENTITY_POSITION, pedId, true);
        game::invoke<void>(nh::SET_ENTITY_COLLISION, pedId, false, false);
        auto heading = game::invoke<f32>(nh::GET_ENTITY_HEADING, pedId);
        Vec3 pos = game::localPos();
        f32 speed = s.noClipSpeed;
        if (GetAsyncKeyState('W') & 0x8000) {
            f32 rad = heading * 3.14159f / 180.f;
            pos.x += sinf(-rad) * speed;
            pos.y += cosf(-rad) * speed;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            f32 rad = heading * 3.14159f / 180.f;
            pos.x -= sinf(-rad) * speed;
            pos.y -= cosf(-rad) * speed;
        }
        if (GetAsyncKeyState(VK_SPACE) & 0x8000)    pos.z += speed;
        if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)  pos.z -= speed;
        game::invoke<void>(nh::SET_ENTITY_COORDS, pedId, pos.x, pos.y, pos.z, false, false, false, false);
    } else {
        static bool wasNoClip = false;
        if (wasNoClip) {
            game::invoke<void>(nh::FREEZE_ENTITY_POSITION, pedId, false);
            game::invoke<void>(nh::SET_ENTITY_COLLISION, pedId, true, true);
        }
        wasNoClip = s.noClip;
    }

    if (s.infiniteAmmo)    game::invoke<void>(nh::SET_PED_INFINITE_AMMO, pedId, true, 0);
    if (s.infiniteClip)    game::invoke<void>(nh::SET_PED_INFINITE_AMMO_CLIP, pedId, true);
    if (s.explosiveAmmo)   game::invoke<void>(nh::SET_EXPLOSIVE_AMMO_THIS_FRAME, pid);
    if (s.fireAmmo)        game::invoke<void>(nh::SET_FIRE_AMMO_THIS_FRAME, pid);
    if (s.explosiveMelee)  game::invoke<void>(nh::SET_EXPLOSIVE_MELEE_THIS_FRAME, pid);

    auto veh = game::localVehicle();
    if (s.vehGodMode && veh) {
        mem::write<f32>(veh + off::Veh_Health,    1000.f);
        mem::write<f32>(veh + off::Veh_HealthMax, 1000.f);
        mem::write<f32>(veh + off::Veh_Body,      1000.f);
        mem::write<f32>(veh + off::Veh_Engine,    1000.f);
        mem::write<f32>(veh + off::Veh_Petrol,    1000.f);
    }
    if (s.seatbelt)
        game::invoke<void>(nh::SET_PED_CONFIG_FLAG, pedId, (i32)32, false);
    if (s.hornBoost && veh && (GetAsyncKeyState('E') & 0x8000))
        game::invoke<void>(nh::SET_VEHICLE_FORWARD_SPEED, pedId, 80.f);
    if (s.freezeTime)
        game::invoke<void>(nh::PAUSE_CLOCK, true);

    game::invoke<void>(nh::SET_NIGHTVISION, s.nightVision);
    game::invoke<void>(nh::SET_SEETHROUGH, s.thermalVision);
}
