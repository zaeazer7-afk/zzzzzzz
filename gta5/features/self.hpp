#pragma once

namespace gta5::features {

void GodMode(bool toggle);
void NeverWanted();
void SetWantedLevel(int level);
void Heal();
void SetArmor(int val);
void SuperJump();
void NoRagdoll(bool toggle);
void Invisible(bool toggle);

void SelfTick(bool godMode, bool neverWanted, bool noRagdoll, bool invisible);

} // namespace gta5::features
