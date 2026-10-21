# Sakura FiveM Internal — Build Guide

## Prerequisites

- Visual Studio 2022 (Desktop C++ workload)
- Windows SDK 10
- CMake 3.20+

## Step 1: Get ImGui

Download ImGui and place these files in `src/imgui/`:

```
src/imgui/
  imgui.h
  imgui.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
  imgui_internal.h
  imconfig.h
  imstb_rectpack.h
  imstb_textedit.h
  imstb_truetype.h
  imgui_impl_win32.h
  imgui_impl_win32.cpp
  imgui_impl_dx11.h
  imgui_impl_dx11.cpp
```

Get them from: https://github.com/ocornut/imgui
- Core files from root
- Backend files from `backends/`

## Step 2: Build the DLL

### Option A: CMake

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Output: `build/Release/sakura.dll`

### Option B: Visual Studio

1. Open VS2022 → File → Open → CMake → select CMakeLists.txt
2. Set config to Release x64
3. Build → Build All

### Option C: Developer Command Prompt

```bash
cl /std:c++20 /EHsc /LD /O2 /DNDEBUG ^
   src/dllmain.cpp src/hooks.cpp src/menu.cpp src/features.cpp ^
   src/imgui/imgui.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_tables.cpp ^
   src/imgui/imgui_widgets.cpp src/imgui/imgui_impl_win32.cpp src/imgui/imgui_impl_dx11.cpp ^
   /Iimgui /link d3d11.lib dxgi.lib user32.lib /OUT:sakura.dll
```

## Step 3: Build the Injector

```bash
cl /std:c++20 /EHsc injector.cpp /link /out:inject.exe
```

## Step 4: Use

1. Launch FiveM, join a server, wait until you're in-game
2. Run `inject.exe` as Administrator
3. Press INSERT to open/close the menu

## Controls

- INSERT — toggle menu
- Mouse — navigate when menu is open (input is captured)
- END — unload (from Misc tab)

## Notes

- The features.cpp tick loop is commented out — it needs a native
  invoker wired in. The menu/overlay renders immediately on inject.
  To make features work, hook into FiveM's scripting runtime or
  use ScriptHookV's native calling mechanism.
- The vtable hook in hooks.cpp patches the dummy swapchain's vtable.
  For FiveM specifically, you may need to hook the ACTUAL swapchain
  that FiveM's renderer creates. Use MinHook for a more reliable
  approach (hook the Present function address directly).
- Run as admin or injection will fail (OpenProcess needs elevated privs).
