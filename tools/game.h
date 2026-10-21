/**
 * game.h — GTA5 Enhanced offsets & signatures
 * Source: YimMenuV2 (github.com/YimMenu/YimMenuV2, Oct 18 2026)
 * Verified by static_assert in YimMenuV2 type headers.
 *
 * IMPORTANT: Patterns are build-specific. After a game update,
 * re-scan with gta5_scanner.py or update from YimMenuV2.
 */
#pragma once
#include <cstdint>

// ============================================================
//  SIGNATURE PATTERNS — from YimMenuV2 Pointers.cpp
//  Format: hex bytes, '?' = wildcard
//  Resolution: Add(N) = offset from match, Rip() = resolve RIP-relative
// ============================================================

namespace Sig {

// --- Rendering / Window ---
// SwapChain: "72 C7 EB 02 31 C0 8B 0D"
//   CommandQueue: Add(0x1A).Add(3).Rip()
//   SwapChain:    Add(0x21).Add(3).Rip()
constexpr auto SwapChain         = "72 C7 EB 02 31 C0 8B 0D";

// WndProc: Sub(0x4F).As()
constexpr auto WndProc           = "3D 85 00 00 00 0F 87 2D 02 00 00";

// ScreenRes: ScreenResX: Add(5).Add(4).Rip()  ScreenResY: Add(0x1E).Add(4).Rip()
constexpr auto ScreenRes         = "75 39 0F 57 C0 F3 0F 2A 05";

// --- Game Version ---
// GameVersion: Add(3).Rip()  OnlineVersion: Add(0x47).Add(3).Rip()
constexpr auto Version           = "4C 8D 0D ? ? ? ? 48 8D 5C 24 ? 48 89 D9 48 89 FA";

// --- Script Engine ---
// ScriptThreads: Add(3).Rip()
constexpr auto ScriptThreads     = "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97";

// InitNativeTables: Sub(0x2A).As()
constexpr auto InitNativeTables  = "EB 2A 0F 1F 40 00 48 8B 54 17 10";

// RunScriptThreads: Sub(0xA).As()
constexpr auto RunScriptThreads  = "BE 40 5D C6 00";

// ScriptGlobals: Add(7).Add(3).Rip()
constexpr auto ScriptGlobals     = "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8";

// ScriptPrograms: Add(0x13).Add(3).Rip().Add(0xD8)
constexpr auto ScriptPrograms    = "48 C7 84 C8 D8 00 00 00 00 00 00 00";

// ScriptVM: Sub(0x24).As()
constexpr auto ScriptVM          = "49 63 41 1C";

// --- Entities / Pools ---
// PedFactory: Add(7).Add(3).Rip()
constexpr auto PedFactory        = "C7 40 30 03 00 00 00 48 8B 0D";

// PedPool: Add(0x18).Add(3).Rip()
constexpr auto PedPool           = "80 79 4B 00 0F 84 F5 00 00 00 48 89 F1";

// VehiclePool: Sub(0xA).Add(3).Rip()
constexpr auto VehiclePool       = "48 83 78 18 0D";

// ObjectPool: Add(5).Add(3).Rip()
constexpr auto ObjectPool        = "48 8B 04 0A C3 0F B6 05";

// GetPoolType: Sub(19).As()
constexpr auto GetPoolType       = "BA CE 94 A6 ED E8";

// HandleToPtr: Add(0x21).Add(1).Rip()  PtrToHandle: Sub(0xB).Add(1).Rip()
constexpr auto HandlesAndPtrs    = "0F 1F 84 00 00 00 00 00 89 F8 0F 28 FE 41";

// --- Network ---
// NetworkPlayerMgr: Add(2).Add(3).Rip()
constexpr auto NetworkPlayerMgr  = "75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00";

// NetworkPlayerMgrInit: As()
constexpr auto NetworkPlayerMgrInit     = "41 56 56 57 55 53 48 83 EC 30 4C 89 CF 45 89 C6 48 89 D3";

// NetworkPlayerMgrShutdown: Sub(0x1B).As()
constexpr auto NetworkPlayerMgrShutdown = "84 C0 0F 84 88 00 00 00 4C 89 F9 E8";

// GetNetPlayerFromPID: Add(8).Rip()
constexpr auto GetNetPlayerFromPID      = "83 FB 20 74 2A 89 D9";

// IsSessionStarted: Add(3).Rip()
constexpr auto IsSessionStarted  = "0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A";

// AssignPhysicalIndex: Sub(0x13).As()
constexpr auto AssignPhysicalIndex = "41 8D 47 01 3C 20 0F 87 68 01 00 00";

// NetworkObjectMgr: Add(0xC).Add(3).Rip()  GetSyncTreeForType: Add(0x13).Add(1).Rip()
constexpr auto NetworkObjectMgr  = "41 83 7E FA 02 40 0F 9C C5 C1 E5 02";

// GetNetObjectById: Add(4).Add(1).Rip()
constexpr auto GetNetObjectById  = "0F B7 4E 60 E8";

// RequestControl: Add(5).Add(1).Rip()
constexpr auto RequestControl    = "74 0A 48 89 F9 E8 ? ? ? ? 31 F6 89 F0 48 83 C4 20";

// ReceiveNetMessage: Add(0xD).Add(1).Rip()
constexpr auto ReceiveNetMessage = "48 81 C1 00 03 00 00 4C 89 E2";

// NetEventMgr: Add(3).Rip()
constexpr auto NetEventMgr      = "4C 8B 05 ? ? ? ? 44 0F B7 CA";

// SendEventAck: EventAck: Add(1).Rip()  SendEventAck: Add(0x1A).Add(1).Rip()
constexpr auto SendEventAck     = "E8 ? ? ? ? 84 C0 75 ? 44 89 F5";

// QueuePacket: Add(3).Add(1).Rip()
constexpr auto QueuePacket      = "45 89 F1 E8 ? ? ? ? 84 C0 74 25";

// NetworkSession: Add(0x17).Add(3).Rip()
constexpr auto NetworkSession    = "49 C7 86 F8 00 00 00 00 00 00 00 48 8B 05";

// JoinSessionByInfo: Add(1).Rip()
constexpr auto JoinSessionByInfo = "E8 ? ? ? ? 0F 10 87 ? ? ? ? 0F 11 86 ? ? ? ? 88 86 ? ? ? ? 84 C0";

// GetSessionByGamerHandle: Sub(0x4A).Add(1).Rip()
constexpr auto GetSessionByGamerHandle = "48 C7 84 24 80 00 00 00 10 00 00 08";

// NetworkTime: Add(2).Rip()
constexpr auto NetworkTime       = "89 05 ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? E9";

// --- Sync Trees ---
// WriteNodeData: As()
constexpr auto WriteNodeData     = "48 8B 89 A8 00 00 00 48 8B 01 48 8B 40 10 48 FF E0";

// ShouldUseNodeCache: Sub(5).As()  Nullsub: Add(0x29).As()
constexpr auto ShouldUseNodeCache = "83 FA 20 74 1D 48 89 CE";

// IsNodeInScope: Sub(0x26).As()
constexpr auto IsNodeInScope     = "41 83 F9 02 74 22 48 8B 06";

// WriteSyncTree: Sub(0x10).As()
constexpr auto WriteSyncTree     = "4D 89 CD 45 89 C6 41 89 D7 48 89 CF 8B 05 ? ? ? ? 65 48 8B 0C 25 58 00 00 00";

// WriteNetArrayData: Sub(0x4E).As()  NetArrayMgr: Sub(0x32).Add(3)
constexpr auto WriteNetArrayData = "0F 84 06 03 00 00 0F B6 83";

// MigrateObject: Add(0x10).Add(1).Rip()
constexpr auto MigrateObject     = "48 8B 96 D0 00 00 00 4C 89 F9 41 B8 03 00 00 00";

// --- Weapon ---
// TriggerWeaponDamageEvent: Add(1).Rip()
constexpr auto TriggerWeaponDamageEvent = "E8 ? ? ? ? 66 90 FF C5";

// --- Anti-Cheat ---
// BEData: BERestartStatus: Add(3).Rip().Add(8).Add(4)
constexpr auto BEData = "48 C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? 48 89 C1 E8 ? ? ? ? E8 ? ? ? ? BD 0A 00 00 00";

// BattlEyeStatusUpdatePatch: Add(11).Rip().Add(1).Rip()
constexpr auto BattlEyeStatusUpdatePatch = "C6 05 ? ? ? ? 00 84 C0 0F 84 ? ? ? ? E9";

// BattlEyeServerProcessPlayerJoin: Sub(4).Rip().As<PVOID*>()[1]
constexpr auto BattlEyeServerProcessPlayerJoin = "48 89 10 48 89 48 10 89 F9";

// AnticheatContext: Sub(0x12).Add(3).Rip()
constexpr auto AnticheatContext  = "48 8D BB 70 0A 00 00 4C 8D 35 ? ? ? ? 66 90";

// AnticheatInitializedHash: Add(9).Rip()
constexpr auto AnticheatInitializedHash = "89 9E C8 00 00 00 48 8B 0D ? ? ? ? 48 85 C9 74 46";

// GetAnticheatInitializedHash2: Add(9).Rip()
constexpr auto GetAnticheatInitializedHash2 = "89 9E E8 00 00 00 89 C2 E8 ? ? ? ? 69";

// --- Misc ---
// RegionCode: Add(3).Rip()
constexpr auto RegionCode        = "4C 8D 05 ? ? ? ? 48 89 F1 48 89 FA E8 ? ? ? ? 84 C0 74 3D";

// QueueDependency: Add(0x71).Add(1).Rip()  SigScanMemory: Add(4).Add(3).Rip()
constexpr auto QueueDependency   = "0F 29 46 50 48 8D 05";

// PrepareMetricForSending: As()
constexpr auto PrepareMetricForSending = "41 56 56 57 55 53 48 83 EC ? 4C 89 CB 4C 89 C6";

// CStatsMgr: Add(4).Add(3).Rip()
constexpr auto CStatsMgr         = "89 6C 24 28 48 8D 0D ? ? ? ? 48 8D";

// GetPackedStatData: Sub(0xE).As()
constexpr auto GetPackedStatData = "8D 81 37 FE FF FF";

// TransactionMgr: Add(3).Rip()
constexpr auto TransactionMgr   = "48 8B 05 ? ? ? ? 80 78 39 00 74 2D";

// GetActiveBasket: Sub(0x39).As()
constexpr auto GetActiveBasket   = "48 8B 40 10 81 7B 0C AE A0 A9 04";

// NetCatalog: Add(0xB).Add(3).Rip()  GetCatalogItem: Add(0x17).Add(1).Rip()
constexpr auto NetCatalog        = "0F 82 55 FF FF FF 44 89 7C 24 30";

// GameTimer: Add(2).Rip()
constexpr auto GameTimer         = "3B 2D ? ? ? ? 76 ? 89 D9";

// GameDataHash: Add(3).Rip()
constexpr auto GameDataHash      = "48 8D 3D ? ? ? ? 69 C9";

// DLCManager: Sub(4).Rip()  GetDLCHash: Add(3).Rip()
constexpr auto GetDLCHash        = "31 D2 E8 ? ? ? ? 3B 84";

// HttpStartRequest: As()
constexpr auto HttpStartRequest  = "56 57 48 83 EC 28 48 89 CE 8B 81 ? ? ? ? FF C8 83 F8 04 0F 87";

// GameSkeletonUpdate: As()
constexpr auto GameSkeletonUpdate = "56 48 83 EC 20 48 8B 81 40 01 00 00 48 85 C0";

// Matchmaking
constexpr auto MatchmakingAdvertise   = "C7 47 30 01 00 00 00 E9 92";   // Sub(0xC).Rip()
constexpr auto MatchmakingUpdate      = "C7 47 30 02 00 00 00 EB 7A";   // Sub(0x8).Rip()
constexpr auto MatchmakingUnadvertise = "C7 86 C8 01 00 00 04 00 00 00"; // Sub(0xC).Rip()

// SessionDetailSendResponse: Add(0x2F).Rip()
constexpr auto SessionDetailSendResponse = "48 B8 01 00 00 00 0D 00 00 00";

// ScMembershipStuff (HasGTAPlus): Add(3).Rip()
constexpr auto ScMembershipStuff = "48 8D 15 ? ? ? ? 41 B8 18 02 00 00 E8";

// CStatsMpCharacterMappingData: Add(3).Rip()
constexpr auto CStatsMpCharacterMappingData = "48 8D 0D ? ? ? ? 89 F2 0F 28 74 24 ? 48 83 C4 38";

// --- Byte Patches ---
// SpectatePatch: As() → patch to 0xEB
constexpr auto SpectatePatch     = "74 ? 66 83 FF 0D";

// ModelSpawnBypass: Add(1).Rip().Add(0x2B) → patch to 0xEB
constexpr auto ModelSpawnBypass  = "E8 ? ? ? ? 48 8B 78 48";

// WorldModelSpawnBypass: Add(4) → patch
constexpr auto WorldModelSpawnBypass = "4C 8B 2C 01 4D 85 ED 0F 84 ? ? ? ?";

// NetArrayCachedDataPatch: As() → patch
constexpr auto NetArrayCachedDataPatch = "0F 84 64 FE FF FF FF";

// AbilityBarPatch: As() → patch to 0x9090
constexpr auto AbilityBarPatch   = "75 39 48 85 F6 74 1A 48 89 F1 E8";

// Aim patches
constexpr auto ShouldNotTargetEntityPatch = "F6 80 A9 14 00 00 01";     // Sub(0x53)
constexpr auto GetAssistedAimTypePatch    = "FF E0 48 8D 86";           // Sub(0x15)
constexpr auto GetLockOnPosPatch          = "0F 29 74 24 ? 48 89 D6 48 89 CF 48 8B 05"; // Add(0x22) → 0xEB
constexpr auto ShouldAllowDriverLockOnPatch = "75 ? 45 89 C7 49 89 CE"; // Sub(0x2C)

// AssistedAim functions
constexpr auto AssistedAimShouldReleaseEntity = "80 7F 28 04 75 6A";     // Sub(0xF).As()
constexpr auto AssistedAimFindNewTarget       = "0F 84 C9 00 00 00 48 89 CE 48 89 F9"; // Sub(0x33).As()

// AllowPausingInSessionPatch: Sub(0x1E) → 0xEB
constexpr auto AllowPausingInSessionPatch = "80 88 ? ? ? ? ? EB ? E8";

// Pool join patches
constexpr auto SetJoinRequestPoolTypePatch    = "89 86 ? ? ? ? E8 ? ? ? ? 89 C0";           // Sub(5)
constexpr auto HandleJoinRequestIgnorePoolPatch = "41 83 FF 05 0F 84 ? ? ? ? 8B 84 F7";     // As()

// FormatInt callers (used for crash patches)
constexpr auto FormatIntCaller1  = "48 89 35 ? ? ? ? 48 8B 74 24"; // Add(0x5D).As()
constexpr auto FormatIntCaller2  = "48 B8 20 73 69 7A 65 3D 27 32 48 89 84 24"; // Sub(0x11).As()

} // namespace Sig


// ============================================================
//  STRUCT OFFSETS — from YimMenuV2 type headers
//  Verified by static_assert(sizeof(...))
// ============================================================

namespace Off {

// --- rage::fwEntity (sizeof 0xC0) ---
// Base of: CEntity → CDynamicEntity → CPed / CVehicle / CObject
namespace fwEntity {
    constexpr uint32_t m_ModelInfo          = 0x20;
    constexpr uint32_t m_Type              = 0x28; // uint8: 3=VEH, 4=PED, 5=OBJ
    constexpr uint32_t m_Flags             = 0x2D;
    constexpr uint32_t m_Navigation        = 0x30;
    constexpr uint32_t m_DynamicEntityComp = 0x40;
    constexpr uint32_t m_DrawData          = 0x48;
    constexpr uint32_t m_Transform         = 0x60; // matrix44, position = row[3] @ 0x90
    constexpr uint32_t m_Position          = 0x90; // vec3 from transform row[3]
    constexpr uint32_t m_RenderFocusEntity = 0xA0;
    constexpr uint32_t m_Flags2            = 0xAC;
    constexpr uint32_t m_ShadowFlags       = 0xB0;
    constexpr uint32_t SIZE                = 0xC0;
}

// --- CEntity (sizeof 0xD0, extends fwEntity) ---
namespace CEntity {
    constexpr uint32_t m_flags_3 = 0xC0;
    constexpr uint32_t m_flags_4 = 0xC4;
    constexpr uint32_t SIZE      = 0xD0;
}

// --- CDynamicEntity (sizeof 0xF0, extends CEntity) ---
namespace CDynamicEntity {
    constexpr uint32_t m_NetObject = 0xD0;
    constexpr uint32_t SIZE        = 0xF0;
}

// --- CPedFactory (sizeof 0x10) ---
namespace CPedFactory {
    constexpr uint32_t m_LocalPed = 0x08;
    constexpr uint32_t SIZE       = 0x10;
}

// --- rage::netObject (sizeof 0x90) ---
namespace netObject {
    constexpr uint32_t m_ObjectType     = 0x08;
    constexpr uint32_t m_ObjectId       = 0x0A;
    constexpr uint32_t m_OwnerId        = 0x49;
    constexpr uint32_t m_NextOwnerId    = 0x4A;
    constexpr uint32_t m_IsRemote       = 0x4B;
    constexpr uint32_t m_Flags          = 0x4C;
    constexpr uint32_t m_Flags2         = 0x4E;
    constexpr uint32_t m_GameObject     = 0x50;
    constexpr uint32_t m_OwnershipToken = 0x88;
    constexpr uint32_t SIZE             = 0x90;
}

// --- rage::netPlayer (sizeof 0xE0) ---
namespace netPlayer {
    constexpr uint32_t m_AccountId          = 0x08;
    constexpr uint32_t m_RockstarId         = 0x10;
    constexpr uint32_t m_PlatformAccountId  = 0x18;
    constexpr uint32_t m_NonPhysicalPlayer  = 0x50;
    constexpr uint32_t m_MessageId          = 0x58;
    constexpr uint32_t m_ActiveIndex        = 0x60;
    constexpr uint32_t m_PlayerIndex        = 0x61;
    constexpr uint32_t m_Flags              = 0xD0;
    constexpr uint32_t SIZE                 = 0xE0;
}

// --- CNetGamePlayer (sizeof 0x370, extends netPlayer) ---
namespace CNetGamePlayer {
    constexpr uint32_t m_Unk        = 0xE0;
    constexpr uint32_t m_PlayerInfo = 0xE8;
    constexpr uint32_t SIZE         = 0x370;
}

// --- rage::netPlayerMgrBase (sizeof 0x8F0) ---
namespace netPlayerMgrBase {
    constexpr uint32_t m_NetConnectionMgr          = 0x08;
    constexpr uint32_t m_BandwidthMgr              = 0x10;
    constexpr uint32_t m_LocalPlayer               = 0xF0;
    constexpr uint32_t m_Players                   = 0x188; // CNetGamePlayer*[32]
    constexpr uint32_t m_MaxPlayers                = 0x288;
    constexpr uint32_t m_UnloadedPlayerCount       = 0x290;
    constexpr uint32_t m_LoadedPlayerCount         = 0x294;
    constexpr uint32_t m_LoadedNonLocalPlayerCount = 0x298;
    constexpr uint32_t m_PhysicalPlayerCount       = 0x29C;
    constexpr uint32_t m_LocalPhysicalPlayerCount  = 0x2A0;
    constexpr uint32_t m_NonLocalPhysicalPlayerCount = 0x2A4;
    constexpr uint32_t SIZE                        = 0x8F0;
}

// --- rage::rlGamerInfoBase (sizeof 0xC0) ---
namespace rlGamerInfoBase {
    constexpr uint32_t m_SecurityEnabled = 0x00;
    constexpr uint32_t m_PeerId         = 0x08;
    constexpr uint32_t m_GamerHandle    = 0x10;
    constexpr uint32_t m_AESKey         = 0x20;
    constexpr uint32_t m_RelayAddress   = 0x48;
    constexpr uint32_t m_RelaySignature = 0x68;
    constexpr uint32_t m_ExternalAddr   = 0xA8;
    constexpr uint32_t m_InternalAddr   = 0xB0;
    constexpr uint32_t m_NatType        = 0xB8;
    constexpr uint32_t m_ForceRelays    = 0xBC;
    constexpr uint32_t SIZE             = 0xC0;
}

// --- rage::rlGamerInfo (sizeof 0xF0, extends rlGamerInfoBase) ---
namespace rlGamerInfo {
    constexpr uint32_t m_HostToken    = 0xC0;
    constexpr uint32_t m_ROSPrivilege = 0xD8;
    constexpr uint32_t m_name         = 0xDC;
    constexpr uint32_t SIZE           = 0xF0;
}

// --- PoolEncryption (sizeof 0x18) ---
namespace PoolEncryption {
    constexpr uint32_t m_IsSet  = 0x00;
    constexpr uint32_t m_First  = 0x08;
    constexpr uint32_t m_Second = 0x10;
    constexpr uint32_t SIZE     = 0x18;
}

// --- rage::fwBasePool (sizeof 0x30) ---
namespace fwBasePool {
    constexpr uint32_t m_Entries       = 0x08;
    constexpr uint32_t m_Flags         = 0x10;
    constexpr uint32_t m_Size          = 0x18;
    constexpr uint32_t m_ItemSize      = 0x1C;
    constexpr uint32_t m_NextSlotIndex = 0x20;
    constexpr uint32_t m_FreeSlotIndex = 0x28;
    constexpr uint32_t SIZE            = 0x30;
}

// --- rage::fwVehiclePool (encrypted pool) ---
namespace fwVehiclePool {
    constexpr uint32_t m_PoolAddress = 0x08; // void**
    constexpr uint32_t m_Size        = 0x10;
    constexpr uint32_t m_BitArray    = 0x38; // uint32_t*
    constexpr uint32_t m_ItemCount   = 0x60;
}

// --- rage::scrThread (sizeof 0x198) ---
namespace scrThread {
    constexpr uint32_t m_Context       = 0x08;  // scrThread::Context (sizeof 0xB0)
    constexpr uint32_t m_Stack         = 0xB8;
    constexpr uint32_t m_ParameterSize = 0xC4;
    constexpr uint32_t m_ParameterLoc  = 0xC8;
    constexpr uint32_t m_ErrorMessage  = 0xD0;
    constexpr uint32_t m_ScriptHash    = 0x150;
    constexpr uint32_t m_ScriptName    = 0x154;
    constexpr uint32_t SIZE            = 0x198;

    namespace Context {
        constexpr uint32_t m_ThreadId       = 0x00; // +0x08 from scrThread base
        constexpr uint32_t m_ScriptHash     = 0x08;
        constexpr uint32_t m_State          = 0x10;
        constexpr uint32_t m_ProgramCounter = 0x14;
        constexpr uint32_t m_FramePointer   = 0x18;
        constexpr uint32_t m_StackPointer   = 0x1C;
        constexpr uint32_t m_TimerA         = 0x20;
        constexpr uint32_t m_TimerB         = 0x24;
        constexpr uint32_t m_WaitTimer      = 0x28;
        constexpr uint32_t m_StackSize      = 0x58;
        constexpr uint32_t SIZE             = 0xB0;
    }
}

// --- rage::scrProgram (sizeof 0x80) ---
namespace scrProgram {
    constexpr uint32_t m_CodeBlocks         = 0x10;
    constexpr uint32_t m_Hash               = 0x18;
    constexpr uint32_t m_CodeSize           = 0x1C;
    constexpr uint32_t m_ArgCount           = 0x20;
    constexpr uint32_t m_LocalCount         = 0x24;
    constexpr uint32_t m_GlobalCount        = 0x28;
    constexpr uint32_t m_NativeCount        = 0x2C;
    constexpr uint32_t m_LocalData          = 0x30;
    constexpr uint32_t m_GlobalData         = 0x38;
    constexpr uint32_t m_NativeEntrypoints  = 0x40;
    constexpr uint32_t m_ProcCount          = 0x48;
    constexpr uint32_t m_ProcNames          = 0x50;
    constexpr uint32_t m_NameHash           = 0x58;
    constexpr uint32_t m_RefCount           = 0x5C;
    constexpr uint32_t m_Name               = 0x60;
    constexpr uint32_t m_StringsData        = 0x68;
    constexpr uint32_t m_StringsCount       = 0x70;
    constexpr uint32_t SIZE                 = 0x80;
}

// --- GtaThread (extends scrThread @ 0x198) ---
namespace GtaThread {
    constexpr uint32_t m_ScriptHandler = 0x198;
    constexpr uint32_t m_NetComponent  = 0x1A0;
    constexpr uint32_t m_ScriptHash2   = 0x1A8;
}

// --- rage::tlsContext (sizeof 0x7B0) ---
namespace tlsContext {
    constexpr uint32_t m_CurrentScriptThread = 0x7A0;
    constexpr uint32_t m_ScriptThreadActive  = 0x7A8;
    constexpr uint32_t SIZE                  = 0x7B0;
}

// --- scriptId (sizeof 0x30) ---
namespace scriptId {
    constexpr uint32_t m_Hash = 0x08;
    constexpr uint32_t m_Name = 0x0C;
    constexpr uint32_t SIZE   = 0x30;
}

// --- CGameScriptId (sizeof 0x40, extends scriptId) ---
namespace CGameScriptId {
    constexpr uint32_t m_Timestamp    = 0x30;
    constexpr uint32_t m_PositionHash = 0x34;
    constexpr uint32_t m_InstanceId   = 0x38;
    constexpr uint32_t m_UniqueId     = 0x3C;
    constexpr uint32_t SIZE           = 0x40;
}

// --- CGameScriptObjInfo (sizeof 0x50) ---
namespace CGameScriptObjInfo {
    constexpr uint32_t m_NetworkId = 0x08;
    constexpr uint32_t m_ScriptId = 0x10;
    constexpr uint32_t SIZE       = 0x50;
}

// --- rage::netGameEvent (sizeof 0x30) ---
namespace netGameEvent {
    constexpr uint32_t m_Type          = 0x08;
    constexpr uint32_t m_RequiresReply = 0x0A;
    constexpr uint32_t m_SourcePlayer  = 0x10;
    constexpr uint32_t m_TargetPlayer  = 0x18;
    constexpr uint32_t m_ResendTime    = 0x20;
    constexpr uint32_t SIZE            = 0x30;
}

// --- CScriptedGameEvent (extends netGameEvent) ---
namespace CScriptedGameEvent {
    constexpr uint32_t m_Args     = 0x70;  // int64[54]
    constexpr uint32_t m_Bitset   = 0x220;
    constexpr uint32_t m_ArgsSize = 0x224;
}

// --- CGameScriptHandlerNetComponent (sizeof 0x1B0) ---
namespace CGameScriptHandlerNetComponent {
    constexpr uint32_t m_State             = 0x08;
    constexpr uint32_t m_FirstParticipant  = 0x10;
    constexpr uint32_t m_Host              = 0x28;
    constexpr uint32_t m_LocalSlotNumber   = 0x30;
    constexpr uint32_t m_ParticipantBitset = 0x38;
    constexpr uint32_t m_HostToken         = 0x3C;
    constexpr uint32_t m_LastHost          = 0x40;
    constexpr uint32_t m_NextHost          = 0x48;
    constexpr uint32_t m_UnkHost           = 0x50;
    constexpr uint32_t m_Participants      = 0x60;  // [32]
    constexpr uint32_t m_NumParticipants   = 0x18C;
    constexpr uint32_t SIZE                = 0x1B0;
}

// --- rage::rlSessionInfo (sizeof 0xD0) ---
namespace rlSessionInfo {
    constexpr uint32_t m_SessionId    = 0x00;
    constexpr uint32_t m_SessionToken = 0x08;
    constexpr uint32_t m_HostInfo     = 0x10;
    constexpr uint32_t SIZE           = 0xD0;
}

// --- rage::netSocketAddress (sizeof 0x08) ---
namespace netSocketAddress {
    constexpr uint32_t m_IpAddress = 0x00;
    constexpr uint32_t m_Port      = 0x04;
    constexpr uint32_t SIZE        = 0x08;
}

// --- rage::netAddress (sizeof 0x20) ---
namespace netAddress {
    constexpr uint32_t m_InternalIp     = 0x00;
    constexpr uint32_t m_ExternalIp     = 0x08;
    constexpr uint32_t m_PeerId        = 0x10;
    constexpr uint32_t m_ConnectionType = 0x1E;
    constexpr uint32_t SIZE             = 0x20;
}

// --- rage::rlGamerHandle (sizeof 0x10) ---
namespace rlGamerHandle {
    constexpr uint32_t m_RockstarId   = 0x00;
    constexpr uint32_t m_Platform     = 0x08;
    constexpr uint32_t m_ProfileIndex = 0x09;
    constexpr uint32_t SIZE           = 0x10;
}

// --- PlatformAccountId (sizeof 0x30) ---
namespace PlatformAccountId {
    constexpr uint32_t m_AccountId = 0x00; // union: XboxUserId/SteamId/EpicAccountId
    constexpr uint32_t m_Platform  = 0x28;
    constexpr uint32_t SIZE        = 0x30;
}

// --- fwExtensibleBase (sizeof 0x20) ---
namespace fwExtensibleBase {
    constexpr uint32_t m_ExtensionContainer = 0x10;
    constexpr uint32_t m_ExtensibleUnk      = 0x18;
    constexpr uint32_t SIZE                 = 0x20;
}

// --- Vehicle Sync Nodes (network serialization, NOT in-memory CVehicle) ---
namespace VehicleSyncNodes {
    namespace CreationData {  // CVehicleCreationDataNode (sizeof 0xE0, base 0xC0)
        constexpr uint32_t m_PopType         = 0xC0;
        constexpr uint32_t m_RandomSeed      = 0xC4;
        constexpr uint32_t m_ModelHash       = 0xC8;
        constexpr uint32_t m_VehicleStatus   = 0xCC;
        constexpr uint32_t m_MaxHealth       = 0xD0;
        constexpr uint32_t m_CreationToken   = 0xD4;
        constexpr uint32_t m_CarBudget       = 0xD8;
        constexpr uint32_t m_NeedsHotwired   = 0xD9;
        constexpr uint32_t m_TiresDontBurst  = 0xDA;
    }
    namespace HealthData {  // CVehicleHealthDataNode (sizeof 0x168)
        constexpr uint32_t m_EngineHealth    = 0x140;
        constexpr uint32_t m_PetrolTankHealth= 0x144;
        constexpr uint32_t m_NumTires        = 0x148;
        constexpr uint32_t m_Health          = 0x150;
        constexpr uint32_t m_BodyHealth      = 0x154;
        constexpr uint32_t m_DamageWeapon    = 0x158;
        constexpr uint32_t m_TotalRepairs    = 0x15E;
    }
}

} // namespace Off
