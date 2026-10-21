# GameDev Toolkit

Framework for building game tools — FiveM, CS2, R6S and more.

## Structure

```
core/           — shared C++ libraries (memory, overlay, math, utils)
fivem/          — FiveM Lua framework, executor, resource dumper
cs2/            — CS2 external tool base + SDK offsets
r6s/            — R6S external tool base + SDK offsets
common/         — cross-game GUI (ImGui menu), config, logging
templates/      — starter templates per game/tool type
generator/      — Python CLI to scaffold new projects
```

## Quick Start

```bash
python generator/generate.py --game fivem --type menu
python generator/generate.py --game cs2 --type esp
python generator/generate.py --game r6s --type aimbot
python generator/generate.py --game fivem --type executor
```

## Build (C++ projects)

Requires: CMake 3.20+, Visual Studio 2022 or g++/clang with C++20.

```bash
cmake -B build -DGAME=CS2
cmake --build build --config Release
```

## FiveM Lua Tools

Drop `fivem/lua_framework/` into your FiveM resources. No C++ build needed.
