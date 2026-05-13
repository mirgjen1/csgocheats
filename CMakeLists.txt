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
    # Internal specific files
    src/memory/internal_memory_reader.cpp
    src/hooks.cpp
    src/rendering/imgui_renderer.cpp
    # ImGui
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/backends/imgui_impl_sdl2.cpp
    thirdparty/imgui/backends/imgui_impl_opengl3.cpp
)

# Note: We don't add the old external opengl_renderer.cpp here 
# as it's not needed for the internal cheat and causes GLFW conflicts.

add_library(csgocheat SHARED ${SOURCES})

target_include_directories(csgocheat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends
)

# Platform-specific dependencies
if(UNIX AND NOT APPLE)
    # Linux/Unix dependencies
    find_package(OpenGL REQUIRED)
    find_package(SDL2 REQUIRED)
    
    # We include SDL2 for headers, but don't link it hard to avoid conflicts.
    # We also link libstdc++ and libgcc statically to avoid runtime version mismatches.
    target_link_libraries(csgocheat PRIVATE 
        OpenGL::OpenGL
        dl
        -static-libstdc++
        -static-libgcc
    )

    target_include_directories(csgocheat PRIVATE
        ${SDL2_INCLUDE_DIRS}
    )
elseif(WIN32)
    # Windows logic...
endif()

# Offset Finder Tool
add_executable(offset_finder
    src/offset_finder_tool.cpp
    src/memory/offset_finder.cpp
)

target_include_directories(offset_finder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(UNIX AND NOT APPLE)
    target_link_libraries(offset_finder PRIVATE X11)
endif()
