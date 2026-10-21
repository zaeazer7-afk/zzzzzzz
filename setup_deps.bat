@echo off
echo Fetching ImGui v1.90.4...
if not exist deps\imgui (
    git clone --depth 1 --branch v1.90.4 https://github.com/ocornut/imgui.git deps/imgui
)
echo Done. Open the project in Visual Studio or build with:
echo   cmake -B build -G "Visual Studio 17 2022" -A x64
echo   cmake --build build --config Release
pause
