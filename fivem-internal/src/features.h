#pragma once
#include "imgui/imgui.h"
#include <string>

namespace Features {
    namespace Self {
        inline bool godMode     = false;
        inline bool semiGod     = false;
        inline bool noRagdoll   = false;
        inline bool invisible   = false;
        inline bool neverWanted = false;
        inline bool superJump   = false;
        inline bool fastRun     = false;
        inline bool noClip      = false;
        inline float speedMult  = 1.0f;
        inline bool maxHealth   = false;
        inline bool maxArmor    = false;
    }

    namespace Weapon {
        inline bool infAmmo       = false;
        inline bool noReload      = false;
        inline bool noRecoil      = false;
        inline bool noSpread      = false;
        inline bool rapidFire     = false;
        inline bool explosiveAmmo = false;
        inline bool fireAmmo      = false;
        inline bool oneShotKill   = false;
        inline bool triggerbot    = false;
        inline bool giveAll       = false;
        inline bool removeAll     = false;
    }

    namespace Vehicle {
        inline bool godMode      = false;
        inline bool speedBoost   = false;
        inline bool superBrakes  = false;
        inline bool noCollision  = false;
        inline bool stickyWheels = false;
        inline bool hornBoost    = false;
        inline float speedMult   = 1.0f;
        inline bool repair       = false;
        inline bool flip         = false;
        inline std::string spawnName;
        inline bool doSpawn      = false;
    }

    namespace Teleport {
        inline bool toWaypoint  = false;
        inline bool toObjective = false;
        inline float tpX = 0, tpY = 0, tpZ = 0;
        inline bool doTp = false;
    }

    namespace Visual {
        inline bool espBoxes    = false;
        inline bool espNames    = false;
        inline bool espHealth   = false;
        inline bool espDistance  = false;
        inline bool espBones    = false;
        inline bool espTracers  = false;
        inline float espRange   = 500.0f;
        inline ImVec4 espColor      = ImVec4(0.85f, 0.25f, 0.45f, 1.0f);
        inline ImVec4 friendlyColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    }

    namespace Online {
        inline bool antiAfk       = false;
        inline bool offRadar      = false;
        inline bool revealHidden  = false;
        inline bool refreshPlayers = false;
        inline int  playerCount   = 0;
        inline char playerNames[256][64] = {};
        inline int  selectedPlayer = -1;
        inline bool doSpectate    = false;
        inline bool doTpToPlayer  = false;
    }

    namespace Lua {
        inline std::string code;
        inline bool doExecute = false;
    }

    namespace Misc {
        inline bool crosshair   = false;
        inline bool watermark   = true;
        inline bool fpsCounter  = false;
        inline float fov        = 90.0f;
        inline float timeScale  = 1.0f;
        inline int weatherIdx   = 0;
        inline bool setWeather  = false;
        inline int hour         = 12;
        inline bool setTime     = false;
        inline bool unload      = false;
    }
}
