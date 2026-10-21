#include "battleye.hpp"
#include "../pointers.hpp"
#include "../../core/memory/patch.hpp"
#include <Windows.h>
#include <cstring>

namespace gta5::hooks {

BattlEye::PatchInfo BattlEye::s_StatusUpdatePatch{};
BattlEye::PatchInfo BattlEye::s_BEDataPatch{};

bool BattlEye::Init() {
    bool ok = true;
    ok &= PatchStatusUpdate();
    ok &= PatchBEData();
    return ok;
}

void BattlEye::Shutdown() {
    if (s_StatusUpdatePatch.applied)
        RestoreBytes(s_StatusUpdatePatch.address, s_StatusUpdatePatch.original, s_StatusUpdatePatch.size);
    if (s_BEDataPatch.applied)
        RestoreBytes(s_BEDataPatch.address, s_BEDataPatch.original, s_BEDataPatch.size);
}

bool BattlEye::PatchStatusUpdate() {
    auto* addr = Pointers::Get().BattlEyeStatusUpdatePatch;
    if (!addr)
        return false;

    // Resolve: Add(11).Rip().Add(1).Rip() -> patch first byte to 0xC3 (ret)
    auto target = core::memory::Address(addr)
        .Add(11).Rip()
        .Add(1).Rip();

    auto* ptr = target.As<uint8_t*>();
    if (!ptr)
        return false;

    s_StatusUpdatePatch.address = ptr;
    s_StatusUpdatePatch.size = 1;
    memcpy(s_StatusUpdatePatch.original, ptr, 1);
    s_StatusUpdatePatch.patched[0] = 0xC3;

    PatchBytes(ptr, s_StatusUpdatePatch.patched, 1);
    s_StatusUpdatePatch.applied = true;
    return true;
}

bool BattlEye::PatchBEData() {
    auto* addr = Pointers::Get().BEData;
    if (!addr)
        return false;

    // BERestartStatus: Add(3).Rip().Add(8).Add(4)
    auto beRestartStatus = core::memory::Address(addr)
        .Add(3).Rip()
        .Add(8)
        .Add(4);

    auto* ptr = beRestartStatus.As<uint8_t*>();
    if (!ptr)
        return false;

    s_BEDataPatch.address = ptr;
    s_BEDataPatch.size = 16;
    memcpy(s_BEDataPatch.original, ptr, 16);
    memset(s_BEDataPatch.patched, 0, 16);

    PatchBytes(ptr, s_BEDataPatch.patched, 16);
    s_BEDataPatch.applied = true;
    return true;
}

void BattlEye::PatchBytes(void* address, const void* patch, size_t size) {
    DWORD oldProtect;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(address, patch, size);
    VirtualProtect(address, size, oldProtect, &oldProtect);
}

void BattlEye::RestoreBytes(void* address, const void* original, size_t size) {
    DWORD oldProtect;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(address, original, size);
    VirtualProtect(address, size, oldProtect, &oldProtect);
}

} // namespace gta5::hooks
