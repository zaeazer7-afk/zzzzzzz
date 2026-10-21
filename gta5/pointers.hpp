#pragma once
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <Windows.h>

namespace gta5 {

struct Pointers {
    // Rendering
    IDXGISwapChain*       SwapChain;
    ID3D12CommandQueue*   CommandQueue;
    void*                 WndProc;
    int*                  ScreenResX;
    int*                  ScreenResY;

    // Version
    const char*           GameVersion;
    const char*           OnlineVersion;

    // Script Engine
    void**                ScriptThreads;
    void*                 InitNativeTables;
    void*                 RunScriptThreads;
    void**                ScriptGlobals;
    void*                 ScriptPrograms;
    void*                 ScriptVM;

    // Entities
    void*                 PedFactory;
    void*                 PedPool;
    void*                 VehiclePool;
    void*                 ObjectPool;
    void*                 GetPoolType;
    void*                 HandleToPtr;
    void*                 PtrToHandle;

    // Network
    void*                 NetworkPlayerMgr;
    void*                 NetworkPlayerMgrInit;
    void*                 NetworkPlayerMgrShutdown;
    void*                 GetNetPlayerFromPID;
    bool*                 IsSessionStarted;
    void*                 AssignPhysicalIndex;
    void*                 NetworkObjectMgr;
    void*                 GetSyncTreeForType;
    void*                 GetNetObjectById;
    void*                 RequestControl;
    void*                 ReceiveNetMessage;
    void*                 NetEventMgr;
    void*                 SendEventAck;
    void*                 QueuePacket;
    void*                 NetworkSession;
    void*                 JoinSessionByInfo;
    void*                 GetSessionByGamerHandle;
    int*                  NetworkTime;

    // Sync
    void*                 WriteNodeData;
    void*                 ShouldUseNodeCache;
    void*                 Nullsub;
    void*                 IsNodeInScope;
    void*                 WriteSyncTree;
    void*                 WriteNetArrayData;
    void*                 NetArrayMgr;
    void*                 MigrateObject;

    // Weapon
    void*                 TriggerWeaponDamageEvent;

    // Anti-cheat
    void*                 BEData;
    void*                 BattlEyeStatusUpdatePatch;
    void*                 BattlEyeServerProcessPlayerJoin;
    void*                 AnticheatContext;
    void*                 AnticheatInitializedHash;

    // Misc
    void*                 RegionCode;
    void*                 QueueDependency;
    void*                 SigScanMemory;
    void*                 PrepareMetricForSending;
    void*                 CStatsMgr;
    void*                 GetPackedStatData;
    void*                 TransactionMgr;
    void*                 GetActiveBasket;
    void*                 NetCatalog;
    void*                 GetCatalogItem;
    int*                  GameTimer;
    void*                 GameDataHash;
    void*                 DLCManager;
    void*                 GetDLCHash;
    void*                 HttpStartRequest;
    void*                 GameSkeletonUpdate;

    // Matchmaking
    void*                 MatchmakingAdvertise;
    void*                 MatchmakingUpdate;
    void*                 MatchmakingUnadvertise;

    // Patch sites
    void*                 SpectatePatch;
    void*                 ModelSpawnBypass;
    void*                 WorldModelSpawnBypass;

    static bool Init();
    static void Shutdown();
    static Pointers& Get();

private:
    static Pointers s_Instance;
};

} // namespace gta5
