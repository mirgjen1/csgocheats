#pragma once

#include <SDL2/SDL.h>

namespace Hooks {
    /**
     * Initialize all hooks
     */
    bool initialize();
    
    /**
     * Restore original functions
     */
    void shutdown();
    
    /**
     * The hooked SDL_GL_SwapWindow function
     */
    void SDL_GL_SwapWindow(SDL_Window* window);
}
