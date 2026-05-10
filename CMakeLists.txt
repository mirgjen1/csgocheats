cmake_minimum_required(VERSION 3.16)
project(CS2Overlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
add_executable(cs2overlay
    src/main.cpp
    src/overlay/overlay.cpp
    src/memory/memory_reader.cpp
    src/memory/game_memory.cpp
    src/rendering/renderer.cpp
    src/rendering/primitives.cpp
    src/entity/entity_manager.cpp
)

target_include_directories(cs2overlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Platform-specific dependencies
if(UNIX AND NOT APPLE)
    target_link_libraries(cs2overlay PRIVATE X11)
elseif(WIN32)
    target_link_libraries(cs2overlay PRIVATE User32 Gdi32)
endif()
