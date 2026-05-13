#include <cstdio>
#include "hooks.hpp"

/**
 * Library constructor - called when the SO is loaded
 */
__attribute__((constructor))
void init_cheat() {
    printf("[Cheat] Shared library loaded. Initializing hooks...\n");
    if (Hooks::initialize()) {
        printf("[Cheat] Hooks initialized successfully.\n");
        printf("[Cheat] Press INSERT in-game to toggle menu.\n");
    } else {
        fprintf(stderr, "[Cheat] Failed to initialize hooks!\n");
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
