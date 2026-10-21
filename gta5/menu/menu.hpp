#pragma once

namespace gta5::menu {

void Init();
void Render();
bool IsOpen();
void Toggle();
void HandleInput(unsigned int msg, unsigned long long wParam, long long lParam);

} // namespace gta5::menu
