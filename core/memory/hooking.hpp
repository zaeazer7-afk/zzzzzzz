#pragma once
#include <cstdint>
#include <Windows.h>
#include <MinHook.h>

namespace core::memory {

class Hooking {
public:
    static bool Init() {
        return MH_Initialize() == MH_OK;
    }

    static void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }

    static bool Create(void* target, void* detour, void** original) {
        if (MH_CreateHook(target, detour, original) != MH_OK)
            return false;
        return MH_EnableHook(target) == MH_OK;
    }

    static bool Remove(void* target) {
        MH_DisableHook(target);
        return MH_RemoveHook(target) == MH_OK;
    }

    static bool Enable(void* target) {
        return MH_EnableHook(target) == MH_OK;
    }

    static bool Disable(void* target) {
        return MH_DisableHook(target) == MH_OK;
    }
};

} // namespace core::memory
