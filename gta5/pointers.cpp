#include "pointers.hpp"
#include "../core/memory/module.hpp"
#include "../core/memory/pattern.hpp"
#include "../core/memory/address.hpp"

using core::memory::Address;
using core::memory::Pattern;
using core::memory::Module;

namespace gta5 {

Pointers Pointers::s_Instance{};

Pointers& Pointers::Get() {
    return s_Instance;
}

void Pointers::Shutdown() {
    s_Instance = {};
}

static void* ScanGame(const Module& mod, const char* pattern) {
    auto result = Pattern::Scan(pattern, mod.Base(), mod.Size());
    if (result) return reinterpret_cast<void*>(*result);
    return nullptr;
}

bool Pointers::Init() {
    Module mod("GTA5_Enhanced.exe");
    if (!mod) return false;

    auto& p = s_Instance;
    memset(&p, 0, sizeof(p));

    // --- Rendering ---
    if (auto r = ScanGame(mod, "72 C7 EB 02 31 C0 8B 0D")) {
        p.CommandQueue = Address(r).Add(0x1A).Add(3).Rip().As<ID3D12CommandQueue*>();
        p.SwapChain    = Address(r).Add(0x21).Add(3).Rip().As<IDXGISwapChain*>();
    }

    if (auto r = ScanGame(mod, "3D 85 00 00 00 0F 87 2D 02 00 00"))
        p.WndProc = Address(r).Sub(0x4F).As<void*>();

    if (auto r = ScanGame(mod, "75 39 0F 57 C0 F3 0F 2A 05")) {
        p.ScreenResX = Address(r).Add(5).Add(4).Rip().As<int*>();
        p.ScreenResY = Address(r).Add(0x1E).Add(4).Rip().As<int*>();
    }

    // --- Version ---
    if (auto r = ScanGame(mod, "4C 8D 0D ? ? ? ? 48 8D 5C 24 ? 48 89 D9 48 89 FA")) {
        p.GameVersion   = Address(r).Add(3).Rip().As<const char*>();
        p.OnlineVersion = Address(r).Add(0x47).Add(3).Rip().As<const char*>();
    }

    // --- Script Engine ---
    if (auto r = ScanGame(mod, "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97"))
        p.ScriptThreads = Address(r).Add(3).Rip().As<void**>();

    if (auto r = ScanGame(mod, "EB 2A 0F 1F 40 00 48 8B 54 17 10"))
        p.InitNativeTables = Address(r).Sub(0x2A).As<void*>();

    if (auto r = ScanGame(mod, "BE 40 5D C6 00"))
        p.RunScriptThreads = Address(r).Sub(0xA).As<void*>();

    if (auto r = ScanGame(mod, "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8"))
        p.ScriptGlobals = Address(r).Add(7).Add(3).Rip().As<void**>();

    if (auto r = ScanGame(mod, "48 C7 84 C8 D8 00 00 00 00 00 00 00"))
        p.ScriptPrograms = Address(r).Add(0x13).Add(3).Rip().Add(0xD8).As<void*>();

    if (auto r = ScanGame(mod, "49 63 41 1C"))
        p.ScriptVM = Address(r).Sub(0x24).As<void*>();

    // --- Entities / Pools ---
    if (auto r = ScanGame(mod, "C7 40 30 03 00 00 00 48 8B 0D"))
        p.PedFactory = Address(r).Add(7).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "80 79 4B 00 0F 84 F5 00 00 00 48 89 F1"))
        p.PedPool = Address(r).Add(0x18).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "48 83 78 18 0D"))
        p.VehiclePool = Address(r).Sub(0xA).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "48 8B 04 0A C3 0F B6 05"))
        p.ObjectPool = Address(r).Add(5).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "BA CE 94 A6 ED E8"))
        p.GetPoolType = Address(r).Sub(19).As<void*>();

    if (auto r = ScanGame(mod, "0F 1F 84 00 00 00 00 00 89 F8 0F 28 FE 41")) {
        p.HandleToPtr = Address(r).Add(0x21).Add(1).Rip().As<void*>();
        p.PtrToHandle = Address(r).Sub(0xB).Add(1).Rip().As<void*>();
    }

    // --- Network ---
    if (auto r = ScanGame(mod, "75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00"))
        p.NetworkPlayerMgr = Address(r).Add(2).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "41 56 56 57 55 53 48 83 EC 30 4C 89 CF 45 89 C6 48 89 D3"))
        p.NetworkPlayerMgrInit = r;

    if (auto r = ScanGame(mod, "84 C0 0F 84 88 00 00 00 4C 89 F9 E8"))
        p.NetworkPlayerMgrShutdown = Address(r).Sub(0x1B).As<void*>();

    if (auto r = ScanGame(mod, "83 FB 20 74 2A 89 D9"))
        p.GetNetPlayerFromPID = Address(r).Add(8).Rip().As<void*>();

    if (auto r = ScanGame(mod, "0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A"))
        p.IsSessionStarted = Address(r).Add(3).Rip().As<bool*>();

    if (auto r = ScanGame(mod, "41 8D 47 01 3C 20 0F 87 68 01 00 00"))
        p.AssignPhysicalIndex = Address(r).Sub(0x13).As<void*>();

    if (auto r = ScanGame(mod, "41 83 7E FA 02 40 0F 9C C5 C1 E5 02")) {
        p.NetworkObjectMgr   = Address(r).Add(0xC).Add(3).Rip().As<void*>();
        p.GetSyncTreeForType = Address(r).Add(0x13).Add(1).Rip().As<void*>();
    }

    if (auto r = ScanGame(mod, "0F B7 4E 60 E8"))
        p.GetNetObjectById = Address(r).Add(4).Add(1).Rip().As<void*>();

    if (auto r = ScanGame(mod, "74 0A 48 89 F9 E8 ? ? ? ? 31 F6 89 F0 48 83 C4 20"))
        p.RequestControl = Address(r).Add(5).Add(1).Rip().As<void*>();

    if (auto r = ScanGame(mod, "48 81 C1 00 03 00 00 4C 89 E2"))
        p.ReceiveNetMessage = Address(r).Add(0xD).Add(1).Rip().As<void*>();

    if (auto r = ScanGame(mod, "4C 8B 05 ? ? ? ? 44 0F B7 CA"))
        p.NetEventMgr = Address(r).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "E8 ? ? ? ? 84 C0 75 ? 44 89 F5"))
        p.SendEventAck = Address(r).Add(0x1A).Add(1).Rip().As<void*>();

    if (auto r = ScanGame(mod, "45 89 F1 E8 ? ? ? ? 84 C0 74 25"))
        p.QueuePacket = Address(r).Add(3).Add(1).Rip().As<void*>();

    if (auto r = ScanGame(mod, "49 C7 86 F8 00 00 00 00 00 00 00 48 8B 05"))
        p.NetworkSession = Address(r).Add(0x17).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "E8 ? ? ? ? 0F 10 87 ? ? ? ? 0F 11 86 ? ? ? ? 88 86 ? ? ? ? 84 C0"))
        p.JoinSessionByInfo = Address(r).Add(1).Rip().As<void*>();

    if (auto r = ScanGame(mod, "48 C7 84 24 80 00 00 00 10 00 00 08"))
        p.GetSessionByGamerHandle = Address(r).Sub(0x4A).Add(1).Rip().As<void*>();

    if (auto r = ScanGame(mod, "89 05 ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? E9"))
        p.NetworkTime = Address(r).Add(2).Rip().As<int*>();

    // --- Sync ---
    if (auto r = ScanGame(mod, "48 8B 89 A8 00 00 00 48 8B 01 48 8B 40 10 48 FF E0"))
        p.WriteNodeData = r;

    if (auto r = ScanGame(mod, "83 FA 20 74 1D 48 89 CE")) {
        p.ShouldUseNodeCache = Address(r).Sub(5).As<void*>();
        p.Nullsub = Address(r).Add(0x29).As<void*>();
    }

    if (auto r = ScanGame(mod, "41 83 F9 02 74 22 48 8B 06"))
        p.IsNodeInScope = Address(r).Sub(0x26).As<void*>();

    if (auto r = ScanGame(mod, "4D 89 CD 45 89 C6 41 89 D7 48 89 CF 8B 05 ? ? ? ? 65 48 8B 0C 25 58 00 00 00"))
        p.WriteSyncTree = Address(r).Sub(0x10).As<void*>();

    if (auto r = ScanGame(mod, "0F 84 06 03 00 00 0F B6 83")) {
        p.WriteNetArrayData = Address(r).Sub(0x4E).As<void*>();
        p.NetArrayMgr = Address(r).Sub(0x32).Add(3).As<void*>();
    }

    if (auto r = ScanGame(mod, "48 8B 96 D0 00 00 00 4C 89 F9 41 B8 03 00 00 00"))
        p.MigrateObject = Address(r).Add(0x10).Add(1).Rip().As<void*>();

    // --- Weapon ---
    if (auto r = ScanGame(mod, "E8 ? ? ? ? 66 90 FF C5"))
        p.TriggerWeaponDamageEvent = Address(r).Add(1).Rip().As<void*>();

    // --- Anti-cheat ---
    if (auto r = ScanGame(mod, "48 C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? 48 89 C1 E8 ? ? ? ? E8 ? ? ? ? BD 0A 00 00 00"))
        p.BEData = r;

    if (auto r = ScanGame(mod, "C6 05 ? ? ? ? 00 84 C0 0F 84 ? ? ? ? E9"))
        p.BattlEyeStatusUpdatePatch = r;

    if (auto r = ScanGame(mod, "48 89 10 48 89 48 10 89 F9"))
        p.BattlEyeServerProcessPlayerJoin = Address(r).Sub(4).Rip().As<void*>();

    if (auto r = ScanGame(mod, "48 8D BB 70 0A 00 00 4C 8D 35 ? ? ? ? 66 90"))
        p.AnticheatContext = Address(r).Sub(0x12).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "89 9E C8 00 00 00 48 8B 0D ? ? ? ? 48 85 C9 74 46"))
        p.AnticheatInitializedHash = Address(r).Add(9).Rip().As<void*>();

    // --- Misc ---
    if (auto r = ScanGame(mod, "4C 8D 05 ? ? ? ? 48 89 F1 48 89 FA E8 ? ? ? ? 84 C0 74 3D"))
        p.RegionCode = Address(r).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "0F 29 46 50 48 8D 05")) {
        p.QueueDependency = Address(r).Add(0x71).Add(1).Rip().As<void*>();
        p.SigScanMemory = Address(r).Add(4).Add(3).Rip().As<void*>();
    }

    if (auto r = ScanGame(mod, "41 56 56 57 55 53 48 83 EC ? 4C 89 CB 4C 89 C6"))
        p.PrepareMetricForSending = r;

    if (auto r = ScanGame(mod, "89 6C 24 28 48 8D 0D ? ? ? ? 48 8D"))
        p.CStatsMgr = Address(r).Add(4).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "8D 81 37 FE FF FF"))
        p.GetPackedStatData = Address(r).Sub(0xE).As<void*>();

    if (auto r = ScanGame(mod, "48 8B 05 ? ? ? ? 80 78 39 00 74 2D"))
        p.TransactionMgr = Address(r).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "48 8B 40 10 81 7B 0C AE A0 A9 04"))
        p.GetActiveBasket = Address(r).Sub(0x39).As<void*>();

    if (auto r = ScanGame(mod, "0F 82 55 FF FF FF 44 89 7C 24 30")) {
        p.NetCatalog = Address(r).Add(0xB).Add(3).Rip().As<void*>();
        p.GetCatalogItem = Address(r).Add(0x17).Add(1).Rip().As<void*>();
    }

    if (auto r = ScanGame(mod, "3B 2D ? ? ? ? 76 ? 89 D9"))
        p.GameTimer = Address(r).Add(2).Rip().As<int*>();

    if (auto r = ScanGame(mod, "48 8D 3D ? ? ? ? 69 C9"))
        p.GameDataHash = Address(r).Add(3).Rip().As<void*>();

    if (auto r = ScanGame(mod, "31 D2 E8 ? ? ? ? 3B 84")) {
        p.DLCManager = Address(r).Sub(4).Rip().As<void*>();
        p.GetDLCHash = Address(r).Add(3).Rip().As<void*>();
    }

    if (auto r = ScanGame(mod, "56 57 48 83 EC 28 48 89 CE 8B 81 ? ? ? ? FF C8 83 F8 04 0F 87"))
        p.HttpStartRequest = r;

    if (auto r = ScanGame(mod, "56 48 83 EC 20 48 8B 81 40 01 00 00 48 85 C0"))
        p.GameSkeletonUpdate = r;

    // Matchmaking
    if (auto r = ScanGame(mod, "C7 47 30 01 00 00 00 E9 92"))
        p.MatchmakingAdvertise = Address(r).Sub(0xC).Rip().As<void*>();

    if (auto r = ScanGame(mod, "C7 47 30 02 00 00 00 EB 7A"))
        p.MatchmakingUpdate = Address(r).Sub(0x8).Rip().As<void*>();

    if (auto r = ScanGame(mod, "C7 86 C8 01 00 00 04 00 00 00"))
        p.MatchmakingUnadvertise = Address(r).Sub(0xC).Rip().As<void*>();

    // Patch sites
    if (auto r = ScanGame(mod, "74 ? 66 83 FF 0D"))
        p.SpectatePatch = r;

    if (auto r = ScanGame(mod, "E8 ? ? ? ? 48 8B 78 48"))
        p.ModelSpawnBypass = Address(r).Add(1).Rip().Add(0x2B).As<void*>();

    if (auto r = ScanGame(mod, "4C 8B 2C 01 4D 85 ED 0F 84 ? ? ? ?"))
        p.WorldModelSpawnBypass = Address(r).Add(4).As<void*>();

    bool ok = p.SwapChain && p.PedFactory && p.ScriptThreads && p.InitNativeTables;
    return ok;
}

} // namespace gta5
