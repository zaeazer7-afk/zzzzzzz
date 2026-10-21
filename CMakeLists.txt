cmake_minimum_required(VERSION 3.20)
project(Trident LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/deps/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/deps/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_dx11.cpp
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_win32.cpp
)

add_library(trident SHARED
    src/dllmain.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(trident PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/deps/imgui
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends
)

target_compile_definitions(trident PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
)

target_link_libraries(trident PRIVATE
    d3d11
    dxgi
    psapi
    dwmapi
)

if(MSVC)
    target_compile_options(trident PRIVATE
        /W3 /EHsc /bigobj
        $<$<CONFIG:Release>:/O2 /GS- /GL /Gy /Oi>
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
    )
    target_link_options(trident PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
endif()

set_target_properties(trident PROPERTIES
    OUTPUT_NAME "trident"
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
