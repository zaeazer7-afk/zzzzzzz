#include "hooking.hpp"
#include <MinHook.h>

namespace core::memory {

bool Hooking::Init() {
    return MH_Initialize() == MH_OK;
}

void Hooking::Shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

} // namespace core::memory
