cmake_minimum_required(VERSION 3.16)
project(CS2Overlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
set(SOURCES
    src/main.cpp
    src/overlay/overlay.cpp
    src/overlay/overlay_window.cpp
    src/memory/memory_reader.cpp
    src/memory/game_memory.cpp
    src/memory/offset_manager.cpp
    src/memory/signature_scanner.cpp
    src/rendering/renderer.cpp
    src/rendering/primitives.cpp
    src/rendering/dx11_renderer.cpp
    src/entity/entity_manager.cpp
)

# Add OpenGL renderer on Linux
if(UNIX AND NOT APPLE)
    list(APPEND SOURCES src/rendering/opengl_renderer.cpp)
endif()

add_executable(cs2overlay ${SOURCES})

target_include_directories(cs2overlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Platform-specific dependencies
if(UNIX AND NOT APPLE)
    # Linux/Unix dependencies
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    
    target_link_libraries(cs2overlay PRIVATE 
        X11 
        OpenGL::OpenGL
        GLEW::GLEW
        glfw
    )
elseif(WIN32)
    target_link_libraries(cs2overlay PRIVATE 
        User32 
        Gdi32 
        d3d11 
        dxgi 
        d3dcompiler
    )
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

