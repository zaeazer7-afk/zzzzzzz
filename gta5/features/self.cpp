#include "self.hpp"
#include "../natives/invoker.hpp"
#include "../natives/hashes.hpp"
#include "../natives/types.hpp"

namespace gta5::features {

using namespace gta5::natives;
using namespace gta5::natives::hash;

static Ped GetPlayerPed() {
    return NativeInvoker::Invoke<Ped>(PLAYER_PED_ID);
}

static Player GetPlayer() {
    return NativeInvoker::Invoke<Player>(PLAYER_ID);
}

void GodMode(bool toggle) {
    Ped ped = GetPlayerPed();
    if (!ped) return;
    NativeInvoker::Invoke<void>(SET_ENTITY_INVINCIBLE, ped, toggle ? TRUE_ : FALSE_);
}

void NeverWanted() {
    Player player = GetPlayer();
    NativeInvoker::Invoke<void>(SET_PLAYER_WANTED_LEVEL, player, 0, FALSE_);
    NativeInvoker::Invoke<void>(SET_PLAYER_WANTED_LEVEL_NOW, player, FALSE_);
    NativeInvoker::Invoke<void>(CLEAR_PLAYER_WANTED_LEVEL, player);
    NativeInvoker::Invoke<void>(SET_MAX_WANTED_LEVEL, 0);
}

void SetWantedLevel(int level) {
    Player player = GetPlayer();
    NativeInvoker::Invoke<void>(SET_PLAYER_WANTED_LEVEL, player, level, FALSE_);
    NativeInvoker::Invoke<void>(SET_PLAYER_WANTED_LEVEL_NOW, player, FALSE_);
}

void Heal() {
    Ped ped = GetPlayerPed();
    if (!ped) return;
    int maxHp = NativeInvoker::Invoke<int>(GET_ENTITY_MAX_HEALTH, ped);
    NativeInvoker::Invoke<void>(SET_ENTITY_HEALTH, ped, maxHp, 0);
}

void SetArmor(int val) {
    Ped ped = GetPlayerPed();
    if (!ped) return;
    NativeInvoker::Invoke<void>(SET_PED_ARMOUR, ped, val);
}

void SuperJump() {
    Ped ped = GetPlayerPed();
    if (!ped) return;
    BOOL jumping = NativeInvoker::Invoke<BOOL>(IS_PED_JUMPING, ped);
    if (!jumping) return;

    Vector3 vel = NativeInvoker::Invoke<Vector3>(GET_ENTITY_VELOCITY, ped);
    if (vel.z > 0.0f && vel.z < 6.0f) {
        vel.z = 16.0f;
        NativeInvoker::Invoke<void>(SET_ENTITY_VELOCITY, ped, vel.x, vel.y, vel.z);
    }
}

void NoRagdoll(bool toggle) {
    Ped ped = GetPlayerPed();
    if (!ped) return;
    NativeInvoker::Invoke<void>(SET_PED_CAN_RAGDOLL, ped, toggle ? FALSE_ : TRUE_);
}

void Invisible(bool toggle) {
    Ped ped = GetPlayerPed();
    if (!ped) return;
    NativeInvoker::Invoke<void>(SET_ENTITY_VISIBLE, ped, toggle ? FALSE_ : TRUE_, FALSE_);
}

void SelfTick(bool godMode, bool neverWanted, bool noRagdoll, bool invisible) {
    if (godMode)      GodMode(true);
    if (neverWanted)  NeverWanted();
    if (noRagdoll)    NoRagdoll(true);
    if (invisible)    Invisible(true);
    SuperJump();
}

} // namespace gta5::features
