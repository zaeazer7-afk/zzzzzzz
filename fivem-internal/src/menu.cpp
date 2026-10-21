// menu.cpp — Sakura FiveM internal menu
#include "menu.h"
#include "features.h"
#include <string>

static bool g_Open = false;
static int  g_Tab  = 0;

void Menu::Toggle() { g_Open = !g_Open; }
bool Menu::IsOpen() { return g_Open; }

void Menu::StyleInit() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    // dark sakura theme
    ImVec4 bg      = ImVec4(0.08f, 0.06f, 0.10f, 0.95f);
    ImVec4 accent  = ImVec4(0.85f, 0.25f, 0.45f, 1.0f);
    ImVec4 accent2 = ImVec4(0.65f, 0.18f, 0.35f, 1.0f);
    ImVec4 text    = ImVec4(0.95f, 0.92f, 0.95f, 1.0f);
    ImVec4 dim     = ImVec4(0.50f, 0.45f, 0.55f, 1.0f);
    ImVec4 panel   = ImVec4(0.12f, 0.09f, 0.14f, 1.0f);

    c[ImGuiCol_WindowBg]          = bg;
    c[ImGuiCol_ChildBg]           = panel;
    c[ImGuiCol_Border]            = ImVec4(accent.x, accent.y, accent.z, 0.3f);
    c[ImGuiCol_FrameBg]           = ImVec4(0.15f, 0.12f, 0.18f, 1.0f);
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.20f, 0.15f, 0.25f, 1.0f);
    c[ImGuiCol_FrameBgActive]     = accent2;
    c[ImGuiCol_TitleBg]           = bg;
    c[ImGuiCol_TitleBgActive]     = ImVec4(0.10f, 0.07f, 0.12f, 1.0f);
    c[ImGuiCol_CheckMark]         = accent;
    c[ImGuiCol_SliderGrab]        = accent;
    c[ImGuiCol_SliderGrabActive]  = accent2;
    c[ImGuiCol_Button]            = ImVec4(accent.x, accent.y, accent.z, 0.25f);
    c[ImGuiCol_ButtonHovered]     = ImVec4(accent.x, accent.y, accent.z, 0.45f);
    c[ImGuiCol_ButtonActive]      = accent;
    c[ImGuiCol_Header]            = ImVec4(accent.x, accent.y, accent.z, 0.20f);
    c[ImGuiCol_HeaderHovered]     = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    c[ImGuiCol_HeaderActive]      = accent;
    c[ImGuiCol_Tab]               = ImVec4(accent.x, accent.y, accent.z, 0.15f);
    c[ImGuiCol_TabHovered]        = ImVec4(accent.x, accent.y, accent.z, 0.40f);
    c[ImGuiCol_TabActive]         = accent2;
    c[ImGuiCol_Text]              = text;
    c[ImGuiCol_TextDisabled]      = dim;
    c[ImGuiCol_ScrollbarBg]       = bg;
    c[ImGuiCol_ScrollbarGrab]     = accent2;
    c[ImGuiCol_Separator]         = ImVec4(accent.x, accent.y, accent.z, 0.2f);

    s.WindowRounding    = 8.0f;
    s.ChildRounding     = 6.0f;
    s.FrameRounding     = 4.0f;
    s.GrabRounding      = 4.0f;
    s.TabRounding       = 4.0f;
    s.WindowPadding     = ImVec2(12, 12);
    s.FramePadding      = ImVec2(8, 4);
    s.ItemSpacing       = ImVec2(8, 6);
    s.ScrollbarSize     = 10.0f;
    s.WindowBorderSize  = 1.0f;
    s.ChildBorderSize   = 1.0f;
}

static void TabSelf() {
    ImGui::Text("SELF");
    ImGui::Separator();
    ImGui::Checkbox("God Mode", &Features::Self::godMode);
    ImGui::Checkbox("Semi-God Mode", &Features::Self::semiGod);
    ImGui::Checkbox("No Ragdoll", &Features::Self::noRagdoll);
    ImGui::Checkbox("Invisible", &Features::Self::invisible);
    ImGui::Checkbox("Never Wanted", &Features::Self::neverWanted);
    ImGui::Checkbox("Super Jump", &Features::Self::superJump);
    ImGui::Checkbox("Fast Run", &Features::Self::fastRun);
    ImGui::Checkbox("No Clip", &Features::Self::noClip);
    ImGui::SliderFloat("Speed Mult", &Features::Self::speedMult, 1.0f, 10.0f);

    ImGui::Spacing();
    ImGui::Text("HEAL / ARMOR");
    ImGui::Separator();
    if (ImGui::Button("Max Health")) Features::Self::maxHealth = true;
    ImGui::SameLine();
    if (ImGui::Button("Max Armor")) Features::Self::maxArmor = true;
}

static void TabWeapon() {
    ImGui::Text("WEAPONS");
    ImGui::Separator();
    ImGui::Checkbox("Infinite Ammo", &Features::Weapon::infAmmo);
    ImGui::Checkbox("No Reload", &Features::Weapon::noReload);
    ImGui::Checkbox("No Recoil", &Features::Weapon::noRecoil);
    ImGui::Checkbox("No Spread", &Features::Weapon::noSpread);
    ImGui::Checkbox("Rapid Fire", &Features::Weapon::rapidFire);
    ImGui::Checkbox("Explosive Ammo", &Features::Weapon::explosiveAmmo);
    ImGui::Checkbox("Fire Ammo", &Features::Weapon::fireAmmo);
    ImGui::Checkbox("One Shot Kill", &Features::Weapon::oneShotKill);
    ImGui::Checkbox("Triggerbot", &Features::Weapon::triggerbot);

    ImGui::Spacing();
    if (ImGui::Button("Give All Weapons")) Features::Weapon::giveAll = true;
    ImGui::SameLine();
    if (ImGui::Button("Remove All")) Features::Weapon::removeAll = true;
}

static void TabVehicle() {
    ImGui::Text("VEHICLE");
    ImGui::Separator();
    ImGui::Checkbox("God Mode", &Features::Vehicle::godMode);
    ImGui::Checkbox("Speed Boost (Shift)", &Features::Vehicle::speedBoost);
    ImGui::Checkbox("Super Brakes", &Features::Vehicle::superBrakes);
    ImGui::Checkbox("No Collision", &Features::Vehicle::noCollision);
    ImGui::Checkbox("Stick to Ground", &Features::Vehicle::stickyWheels);
    ImGui::Checkbox("Horn Boost", &Features::Vehicle::hornBoost);
    ImGui::SliderFloat("Speed Mult", &Features::Vehicle::speedMult, 1.0f, 20.0f);

    ImGui::Spacing();
    if (ImGui::Button("Repair Vehicle")) Features::Vehicle::repair = true;
    ImGui::SameLine();
    if (ImGui::Button("Flip Vehicle")) Features::Vehicle::flip = true;

    ImGui::Spacing();
    ImGui::Text("SPAWNER");
    ImGui::Separator();
    static char spawnBuf[64] = "adder";
    ImGui::InputText("Model", spawnBuf, sizeof(spawnBuf));
    if (ImGui::Button("Spawn")) {
        Features::Vehicle::spawnName = spawnBuf;
        Features::Vehicle::doSpawn = true;
    }
}

static void TabTeleport() {
    ImGui::Text("TELEPORT");
    ImGui::Separator();
    if (ImGui::Button("Waypoint"))   Features::Teleport::toWaypoint = true;
    if (ImGui::Button("Objective"))  Features::Teleport::toObjective = true;

    ImGui::Spacing();
    ImGui::Text("SAVED LOCATIONS");
    ImGui::Separator();
    struct Loc { const char* n; float x, y, z; };
    static Loc locs[] = {
        {"Airport",       -1336.0f, -3044.0f, 13.9f},
        {"Maze Bank Tower", -75.0f,  -818.0f, 326.2f},
        {"Military Base", -2047.0f, 3132.0f,  32.8f},
        {"Mount Chiliad",  450.7f,  5566.6f,  806.2f},
        {"Casino",         924.9f,   46.1f,   81.1f},
        {"LS Customs",    -365.4f, -131.8f,   37.9f},
    };
    for (auto& l : locs) {
        if (ImGui::Button(l.n)) {
            Features::Teleport::tpX = l.x;
            Features::Teleport::tpY = l.y;
            Features::Teleport::tpZ = l.z;
            Features::Teleport::doTp = true;
        }
    }

    ImGui::Spacing();
    ImGui::Text("CUSTOM");
    ImGui::Separator();
    static float cx = 0, cy = 0, cz = 0;
    ImGui::InputFloat("X", &cx);
    ImGui::InputFloat("Y", &cy);
    ImGui::InputFloat("Z", &cz);
    if (ImGui::Button("Teleport##custom")) {
        Features::Teleport::tpX = cx;
        Features::Teleport::tpY = cy;
        Features::Teleport::tpZ = cz;
        Features::Teleport::doTp = true;
    }
}

static void TabVisual() {
    ImGui::Text("VISUALS");
    ImGui::Separator();
    ImGui::Checkbox("ESP - Boxes", &Features::Visual::espBoxes);
    ImGui::Checkbox("ESP - Names", &Features::Visual::espNames);
    ImGui::Checkbox("ESP - Health", &Features::Visual::espHealth);
    ImGui::Checkbox("ESP - Distance", &Features::Visual::espDistance);
    ImGui::Checkbox("ESP - Bones/Skeleton", &Features::Visual::espBones);
    ImGui::Checkbox("ESP - Tracers", &Features::Visual::espTracers);
    ImGui::SliderFloat("ESP Range", &Features::Visual::espRange, 50.0f, 2000.0f);

    ImGui::Spacing();
    ImGui::ColorEdit4("Box Color", &Features::Visual::espColor.x);
    ImGui::ColorEdit4("Friendly Color", &Features::Visual::friendlyColor.x);
}

static void TabOnline() {
    ImGui::Text("ONLINE");
    ImGui::Separator();
    ImGui::Checkbox("Anti-AFK", &Features::Online::antiAfk);
    ImGui::Checkbox("Off Radar", &Features::Online::offRadar);
    ImGui::Checkbox("Reveal Hidden", &Features::Online::revealHidden);

    ImGui::Spacing();
    ImGui::Text("PLAYER LIST");
    ImGui::Separator();
    if (ImGui::Button("Refresh")) Features::Online::refreshPlayers = true;
    for (int i = 0; i < Features::Online::playerCount; i++) {
        if (ImGui::Selectable(Features::Online::playerNames[i],
                              Features::Online::selectedPlayer == i))
            Features::Online::selectedPlayer = i;
    }

    if (Features::Online::selectedPlayer >= 0) {
        ImGui::Spacing();
        if (ImGui::Button("Spectate"))  Features::Online::doSpectate = true;
        ImGui::SameLine();
        if (ImGui::Button("TP to Player")) Features::Online::doTpToPlayer = true;
    }
}

static void TabLua() {
    ImGui::Text("LUA EXECUTOR");
    ImGui::Separator();
    static char luaBuf[4096] = "";
    ImGui::InputTextMultiline("##lua", luaBuf, sizeof(luaBuf),
                               ImVec2(-1, ImGui::GetContentRegionAvail().y - 40));
    if (ImGui::Button("Execute", ImVec2(120, 30))) {
        Features::Lua::code = luaBuf;
        Features::Lua::doExecute = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(80, 30))) {
        luaBuf[0] = '\0';
    }
}

static void TabMisc() {
    ImGui::Text("MISC");
    ImGui::Separator();
    ImGui::Checkbox("Crosshair", &Features::Misc::crosshair);
    ImGui::Checkbox("Watermark", &Features::Misc::watermark);
    ImGui::Checkbox("FPS Counter", &Features::Misc::fpsCounter);
    ImGui::SliderFloat("FOV", &Features::Misc::fov, 50.0f, 130.0f);
    ImGui::SliderFloat("Time Scale", &Features::Misc::timeScale, 0.1f, 2.0f);

    ImGui::Spacing();
    ImGui::Text("WEATHER");
    ImGui::Separator();
    const char* weathers[] = {"CLEAR","EXTRASUNNY","CLOUDS","OVERCAST",
                               "RAIN","CLEARING","THUNDER","SMOG",
                               "FOGGY","XMAS","SNOWLIGHT","BLIZZARD"};
    static int selWeather = 0;
    ImGui::Combo("Weather", &selWeather, weathers, IM_ARRAYSIZE(weathers));
    if (ImGui::Button("Set Weather")) {
        Features::Misc::weatherIdx = selWeather;
        Features::Misc::setWeather = true;
    }

    ImGui::Spacing();
    ImGui::SliderInt("Hour", &Features::Misc::hour, 0, 23);
    if (ImGui::Button("Set Time")) Features::Misc::setTime = true;

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("Unload Menu (END key)")) {
        Features::Misc::unload = true;
    }
}

void Menu::Render() {
    if (!g_Open) {
        // watermark always visible
        if (Features::Misc::watermark) {
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowSize(ImVec2(180, 0));
            ImGui::Begin("##wm", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
            ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.45f, 1.0f), "SAKURA");
            ImGui::SameLine();
            ImGui::TextDisabled("| FiveM");
            ImGui::End();
        }
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(620, 480), ImGuiCond_FirstUseEver);
    ImGui::Begin("SAKURA", &g_Open,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    // header
    ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.45f, 1.0f), "SAKURA");
    ImGui::SameLine();
    ImGui::TextDisabled("FiveM Internal | INSERT to toggle");
    ImGui::Separator();

    // tabs
    const char* tabs[] = {"Self","Weapons","Vehicle","Teleport","Visuals","Online","Lua","Misc"};
    for (int i = 0; i < IM_ARRAYSIZE(tabs); i++) {
        if (i > 0) ImGui::SameLine();
        bool sel = (g_Tab == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.25f, 0.45f, 0.6f));
        if (ImGui::Button(tabs[i], ImVec2(68, 28))) g_Tab = i;
        if (sel) ImGui::PopStyleColor();
    }

    ImGui::Separator();

    // content
    ImGui::BeginChild("##content", ImVec2(0, 0), true);
    switch (g_Tab) {
        case 0: TabSelf();     break;
        case 1: TabWeapon();   break;
        case 2: TabVehicle();  break;
        case 3: TabTeleport(); break;
        case 4: TabVisual();   break;
        case 5: TabOnline();   break;
        case 6: TabLua();      break;
        case 7: TabMisc();     break;
    }
    ImGui::EndChild();

    ImGui::End();
}
