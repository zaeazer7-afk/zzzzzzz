cmake_minimum_required(VERSION 3.20)
project(enhanced LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT for manual map injection compatibility
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ImGui — fetch from GitHub
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_MakeAvailable(imgui)

# ImGui library (core + DX12 + Win32 backends)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# enhanced.dll — the mod menu
add_library(enhanced SHARED
    src/main.cpp
    src/game.cpp
    src/render.cpp
    src/menu.cpp
    src/battleye.cpp
)
target_include_directories(enhanced PRIVATE src)
target_link_libraries(enhanced PRIVATE
    imgui
    d3d12 dxgi d3dcompiler
)
target_precompile_headers(enhanced PRIVATE src/pch.h)
set_target_properties(enhanced PROPERTIES
    OUTPUT_NAME "enhanced"
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# injector.exe — standalone manual mapper
add_executable(injector src/injector.cpp)
set_target_properties(injector PROPERTIES
    OUTPUT_NAME "injector"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
