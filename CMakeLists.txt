cmake_minimum_required(VERSION 3.16)
project(CS2Overlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main library
set(SOURCES
    src/memory/memory_reader.cpp
    src/memory/game_memory.cpp
    src/memory/offset_manager.cpp
    src/memory/signature_scanner.cpp
    src/rendering/renderer.cpp
    src/rendering/primitives.cpp
    src/rendering/esp_ui.cpp
    src/entity/entity_manager.cpp
    src/rendering/ui_backend.cpp
    src/memory/internal_memory_reader.cpp
    src/hooks.cpp
    src/rendering/imgui_renderer.cpp
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/backends/imgui_impl_sdl2.cpp
    thirdparty/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(csgocheat SHARED ${SOURCES})

# Minimal includes - we assume headers are in standard locations
target_include_directories(csgocheat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends
    /usr/include/SDL2
)

# No find_package needed - we rely on the game to provide symbols
# We only link against 'dl' for dlsym
target_link_libraries(csgocheat PRIVATE 
    dl
    -static-libstdc++
    -static-libgcc
)

# Force 32-bit if needed by the user via command line
# or just let it inherit from CMAKE_CXX_FLAGS
