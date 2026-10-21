cmake_minimum_required(VERSION 3.20)
project(sakura-fivem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# output DLL
add_library(sakura SHARED
    src/dllmain.cpp
    src/hooks.cpp
    src/menu.cpp
    src/features.cpp
    # ImGui sources — drop imgui into src/imgui/
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_impl_win32.cpp
    src/imgui/imgui_impl_dx11.cpp
)

target_include_directories(sakura PRIVATE src)
target_link_libraries(sakura PRIVATE d3d11 dxgi)
set_target_properties(sakura PROPERTIES OUTPUT_NAME "sakura" SUFFIX ".dll")
