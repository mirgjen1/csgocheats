#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <iostream>
#include "hooks.hpp"

/**
 * Library constructor - called when the SO is loaded
 */
__attribute__((constructor))
void init_cheat() {
    std::cout << "[Cheat] Shared library loaded. Initializing hooks..." << std::endl;
    if (Hooks::initialize()) {
        std::cout << "[Cheat] Hooks initialized successfully." << std::endl;
        std::cout << "[Cheat] Press INSERT in-game to toggle menu." << std::endl;
    } else {
        std::cerr << "[Cheat] Failed to initialize hooks!" << std::endl;
    }
}

/**
 * Library destructor - called when the SO is unloaded
 */
__attribute__((destructor))
void shutdown_cheat() {
    std::cout << "[Cheat] Shared library unloading. Shutting down hooks..." << std::endl;
    Hooks::shutdown();
    std::cout << "[Cheat] Shutdown complete." << std::endl;
}
