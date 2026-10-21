#pragma once
#include <cstdint>

// Native hashes for GTA5 Enhanced — joaat hashes from nativedb
// https://nativedb.dotindustries.dev/gta5

namespace gta5::natives::hash {

// ---- PLAYER ----
constexpr uint64_t PLAYER_ID                      = 0x4F8644AF03DEB29B;
constexpr uint64_t PLAYER_PED_ID                  = 0xD80958FC74E988A6;
constexpr uint64_t GET_PLAYER_PED                 = 0x43A66C31C68491C0;
constexpr uint64_t GET_PLAYER_NAME                = 0x6D0DE6A7B5DA71F8;
constexpr uint64_t SET_PLAYER_WANTED_LEVEL        = 0x39FF19C64EF7DA5B;
constexpr uint64_t GET_PLAYER_WANTED_LEVEL        = 0xE28E54788CE8F12D;
constexpr uint64_t CLEAR_PLAYER_WANTED_LEVEL      = 0xB302540597885499;
constexpr uint64_t SET_PLAYER_INVINCIBLE          = 0x239528EACDC3E7DE;
constexpr uint64_t SET_MAX_WANTED_LEVEL           = 0xAA5F02DB8F204E3A;
constexpr uint64_t IS_PLAYER_FREE_AIMING          = 0x2E397FD2ECD37C87;
constexpr uint64_t GET_PLAYER_GROUP               = 0x0D127585F77030AF;

// ---- PED ----
constexpr uint64_t IS_PED_IN_ANY_VEHICLE          = 0x997ABD671D25CA0B;
constexpr uint64_t SET_PED_CAN_RAGDOLL            = 0xB128377056A54E2A;
constexpr uint64_t GET_PED_ARMOUR                 = 0x9483AF821C54B65C;
constexpr uint64_t SET_PED_ARMOUR                 = 0xCEA04D83135264CC;
constexpr uint64_t SET_PED_MAX_HEALTH             = 0xF5F6378C4F3419D3;
constexpr uint64_t GET_PED_MAX_HEALTH             = 0x4700A416E8324EF3;
constexpr uint64_t IS_PED_DEAD_OR_DYING           = 0x3317DEDB88C95038;
constexpr uint64_t IS_PED_A_PLAYER                = 0x12534C348C6CB68B;
constexpr uint64_t SET_PED_INTO_VEHICLE           = 0xF75B0D629E1C063D;
constexpr uint64_t SET_PED_NEVER_LEAVES_GROUP     = 0x4684D24D03A021E9;
constexpr uint64_t RESURRECT_PED                  = 0x71BC8E838B9C6035;
constexpr uint64_t SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE = 0x7A6535691B477C48;
constexpr uint64_t CLEAR_PED_TASKS_IMMEDIATELY    = 0xAAA34F8A7CB32098;
constexpr uint64_t GET_PED_NEARBY_PEDS            = 0x23F8F5FC7E8C4A6B;
constexpr uint64_t GET_PED_NEARBY_VEHICLES        = 0xCFF869CBFA210D82;

// ---- ENTITY ----
constexpr uint64_t GET_ENTITY_HEALTH              = 0xEEF059FAD016D209;
constexpr uint64_t SET_ENTITY_HEALTH              = 0x6B76DC1F3AE6E6A3;
constexpr uint64_t GET_ENTITY_COORDS              = 0x3FEF770D40960D5A;
constexpr uint64_t SET_ENTITY_COORDS              = 0x06843DA7060A026B;
constexpr uint64_t SET_ENTITY_VISIBLE             = 0xEA1C610A04DB6BBB;
constexpr uint64_t FREEZE_ENTITY_POSITION         = 0x428CA6DBD1094446;
constexpr uint64_t SET_ENTITY_INVINCIBLE          = 0x3882114BDE571AD4;
constexpr uint64_t SET_ENTITY_COLLISION           = 0x1A9205C1B9EE827F;
constexpr uint64_t GET_ENTITY_SPEED               = 0xD5037BA82E12416F;
constexpr uint64_t GET_ENTITY_MODEL               = 0x9F47B058362C84B5;
constexpr uint64_t GET_ENTITY_HEADING             = 0xE83D4F9BA2A38914;
constexpr uint64_t SET_ENTITY_HEADING             = 0x8E2530AA8ADA980E;
constexpr uint64_t IS_ENTITY_DEAD                 = 0x5F9532F3B5CC2551;
constexpr uint64_t GET_ENTITY_ROTATION            = 0xAFBD61CC738D9EB9;
constexpr uint64_t SET_ENTITY_ROTATION            = 0x8524A8B0171D7EA0;
constexpr uint64_t APPLY_FORCE_TO_ENTITY          = 0xC5F68BE9613E2D18;
constexpr uint64_t SET_ENTITY_MAX_SPEED           = 0x0E46A3FCBDE2A1B1;
constexpr uint64_t DELETE_ENTITY                  = 0xAE3CBE5BF394C9C9;
constexpr uint64_t DOES_ENTITY_EXIST              = 0x7239B21A38F536BA;
constexpr uint64_t IS_ENTITY_IN_WATER             = 0xCFB0A0D8EDD145A3;
constexpr uint64_t SET_ENTITY_ALPHA               = 0x44A0870B7E92D7C0;
constexpr uint64_t SET_ENTITY_NO_COLLISION_ENTITY = 0xA53ED5520C07654A;

// ---- VEHICLE ----
constexpr uint64_t CREATE_VEHICLE                 = 0xAF35D0D2583051B0;
constexpr uint64_t SET_VEHICLE_ON_GROUND_PROPERLY = 0x49733E92263139D1;
constexpr uint64_t SET_VEHICLE_ENGINE_ON          = 0x2497C4717C8B881E;
constexpr uint64_t SET_VEHICLE_FIXED              = 0x115722B1B9C14C1C;
constexpr uint64_t SET_VEHICLE_COLOURS            = 0x4F1D4BE3A7F24601;
constexpr uint64_t SET_VEHICLE_MOD_KIT            = 0x1F2AA07F00B3217A;
constexpr uint64_t SET_VEHICLE_MOD                = 0x6AF0636BBEDCF025;
constexpr uint64_t GET_VEHICLE_MOD                = 0x772960298DA26FDB;
constexpr uint64_t SET_VEHICLE_NUMBER_PLATE_TEXT  = 0x95A88F0B409CDA47;
constexpr uint64_t SET_VEHICLE_FORWARD_SPEED      = 0xAB54A438726D25D5;
constexpr uint64_t IS_VEHICLE_DRIVEABLE           = 0x4C241E39B23DF959;
constexpr uint64_t GET_PED_IN_VEHICLE_SEAT        = 0xBB40DD2270B65366;
constexpr uint64_t SET_VEHICLE_CUSTOM_PRIMARY_COLOUR   = 0x7141766F91D15BEA;
constexpr uint64_t SET_VEHICLE_CUSTOM_SECONDARY_COLOUR = 0x36CED73BFED89754;
constexpr uint64_t SET_VEHICLE_EXTRA_COLOURS      = 0x2036F561ADD12E33;
constexpr uint64_t SET_VEHICLE_WINDOW_TINT        = 0x57C51E6BAD752696;
constexpr uint64_t SET_VEHICLE_TYRES_CAN_BURST    = 0xEB702CA6B35765A4;
constexpr uint64_t SET_VEHICLE_DOORS_LOCKED       = 0xB664292EAECF7FA6;
constexpr uint64_t GET_LAST_DRIVEN_VEHICLE        = 0xB2D06FAEDE65B577;

// ---- WEAPON ----
constexpr uint64_t GIVE_WEAPON_TO_PED             = 0xBF0FD6E56C964FCB;
constexpr uint64_t REMOVE_ALL_PED_WEAPONS         = 0xF25DF915FA38C5F3;
constexpr uint64_t GET_CURRENT_PED_WEAPON         = 0x3A87E44BB9A01D54;
constexpr uint64_t SET_PED_INFINITE_AMMO          = 0x3EDCB0505123623B;
constexpr uint64_t SET_PED_INFINITE_AMMO_CLIP     = 0x183DADC6AA953186;
constexpr uint64_t GET_MAX_AMMO                   = 0xDC16122C7A20C933;
constexpr uint64_t SET_CURRENT_PED_WEAPON         = 0xADF692B254977C0C;
constexpr uint64_t GIVE_DELAYED_WEAPON_TO_PED     = 0xB282DC6EBD803C75;
constexpr uint64_t REMOVE_WEAPON_FROM_PED         = 0x4899CB088EDF59B8;
constexpr uint64_t HAS_PED_GOT_WEAPON             = 0x8DECB02F88F428BC;
constexpr uint64_t SET_PED_DROPS_WEAPONS_WHEN_DEAD = 0x476AE72C1D19D1A8;

// ---- MISC ----
constexpr uint64_t GET_HASH_KEY                   = 0xD24D37CC275948CC;
constexpr uint64_t GET_GROUND_Z_FOR_3D_COORD      = 0xC906A7DAB05C8D2B;
constexpr uint64_t CLEAR_AREA_OF_PEDS             = 0xBE31FD6CE464AC59;
constexpr uint64_t CLEAR_AREA_OF_VEHICLES         = 0x01C7B305B3F3DA58;
constexpr uint64_t GET_GAME_TIMER                 = 0x9CD27B0045628463;
constexpr uint64_t GET_FRAME_COUNT                = 0xFC8202EFC642E6F2;
constexpr uint64_t SET_TIME_SCALE                 = 0x1D408577D440E81E;

// ---- STREAMING ----
constexpr uint64_t REQUEST_MODEL                  = 0x963D27A58DF860AC;
constexpr uint64_t HAS_MODEL_LOADED               = 0x98A4EB5D89A0C952;
constexpr uint64_t SET_MODEL_AS_NO_LONGER_NEEDED  = 0xE532F5D78798DAAB;
constexpr uint64_t IS_MODEL_VALID                 = 0xC0296A2EDF545E92;
constexpr uint64_t IS_MODEL_IN_CDIMAGE            = 0x35B9E0803292B641;
constexpr uint64_t IS_MODEL_A_VEHICLE             = 0xA796899A20B7A7DF;

// ---- HUD ----
constexpr uint64_t BEGIN_TEXT_COMMAND_DISPLAY_TEXT    = 0x25FBB336DF1804CB;
constexpr uint64_t END_TEXT_COMMAND_DISPLAY_TEXT      = 0xCD015E5BB0D96A57;
constexpr uint64_t ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME = 0x6C188BE134E074AA;
constexpr uint64_t SET_TEXT_SCALE                    = 0x07C837F9A01C34C9;
constexpr uint64_t SET_TEXT_COLOUR                   = 0xBE6B23FFA53FB442;
constexpr uint64_t SET_TEXT_OUTLINE                  = 0x2513DFB0FB8400FE;
constexpr uint64_t SET_TEXT_FONT                     = 0x66E0276CC5F6B9DA;

// ---- STATS ----
constexpr uint64_t STAT_SET_INT                   = 0xB3271D7AB655B441;
constexpr uint64_t STAT_GET_INT                   = 0x767FBC2AC802EF3D;
constexpr uint64_t STAT_SET_FLOAT                 = 0x4851997F37FE9B3C;
constexpr uint64_t STAT_SET_BOOL                  = 0x4B33C4243DE0C432;

// ---- GAMEPLAY / CAM ----
constexpr uint64_t GET_GAMEPLAY_CAM_COORD         = 0x14D6F5678D8F1B37;
constexpr uint64_t GET_GAMEPLAY_CAM_ROT           = 0x837765A25378F0BB;
constexpr uint64_t GET_GAMEPLAY_CAM_FOV           = 0x65019750A0324133;

// ---- NETWORK ----
constexpr uint64_t NETWORK_IS_SESSION_STARTED     = 0x9DE624D2FC4B603F;
constexpr uint64_t NETWORK_IS_HOST                = 0x8DB296B814EDDA07;
constexpr uint64_t NETWORK_IS_PLAYER_ACTIVE       = 0xB8DFD30D6973E135;

// ---- OBJECT ----
constexpr uint64_t CREATE_OBJECT                  = 0x509D5878EB39E842;
constexpr uint64_t DELETE_OBJECT                  = 0x539E0AE3E6634B9F;

// ---- GRAPHICS ----
constexpr uint64_t DRAW_MARKER                    = 0x28477EC23D892089;
constexpr uint64_t DRAW_LINE                      = 0x6B7256074AE34680;

// ---- TIME ----
constexpr uint64_t SET_CLOCK_TIME                 = 0x47C3B5848C3E45D8;
constexpr uint64_t GET_CLOCK_HOURS                = 0x25223CA6B4D20B7F;
constexpr uint64_t GET_CLOCK_MINUTES              = 0x13571E1C03373E5E;
constexpr uint64_t PAUSE_CLOCK                    = 0x4055E40BD2DBEC1D;

// ---- WEATHER ----
constexpr uint64_t SET_WEATHER_TYPE_NOW           = 0xED712CA327900C8A;
constexpr uint64_t SET_WEATHER_TYPE_PERSIST       = 0xFB5045B7C42B75BF;
constexpr uint64_t CLEAR_WEATHER_TYPE_PERSIST     = 0xCCC39339BEF76CF5;

// ---- TELEPORT ----
constexpr uint64_t SET_PED_COORDS_KEEP_VEHICLE    = 0x9AFEFF481A85AB2E;

// ---- MAP ----
constexpr uint64_t SET_NEW_WAYPOINT               = 0xFE43368D2AA4F2FC;
constexpr uint64_t GET_FIRST_BLIP_INFO_ID         = 0x1BEDE233E6CD2A1F;
constexpr uint64_t GET_BLIP_INFO_ID_COORD         = 0xFA7C7F0AADF25D09;
constexpr uint64_t IS_WAYPOINT_ACTIVE             = 0x202B1BBFC6AB5EE4;

} // namespace gta5::natives::hash
