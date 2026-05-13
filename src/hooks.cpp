#include "hooks.hpp"
#include <dlfcn.h>
#include <iostream>
#include <memory>
#include "rendering/imgui_renderer.hpp"
#include "rendering/esp_ui.hpp"
#include "entity/entity_manager.hpp"
#include "memory/internal_memory_reader.hpp"
#include "memory/game_memory.hpp"

// Original function pointer type
typedef void (*SDL_GL_SwapWindow_t)(SDL_Window*);
static SDL_GL_SwapWindow_t original_SDL_GL_SwapWindow = nullptr;

// Global cheat components
static std::shared_ptr<ImGuiRenderer> g_renderer = nullptr;
static std::shared_ptr<ESPUI> g_esp_ui = nullptr;
static std::shared_ptr<EntityManager> g_entity_manager = nullptr;
static std::shared_ptr<GameMemory> g_game_memory = nullptr;
static bool g_initialized = false;

extern "C" {
    // This is the function that will be called by the game
    void SDL_GL_SwapWindow(SDL_Window* window) {
        if (!original_SDL_GL_SwapWindow) {
            original_SDL_GL_SwapWindow = (SDL_GL_SwapWindow_t)dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
        }
        
        if (!original_SDL_GL_SwapWindow) return; // Fallback if dlsym fails
        
        if (!g_initialized) {
            printf("[Hooks] Initializing internal cheat components...\n");
            
            // Initialize memory reader (internal)
            auto reader = std::make_shared<InternalMemoryReader>();
            g_game_memory = std::make_shared<GameMemory>(reader);
            
            // Initialize renderer
            g_renderer = std::make_shared<ImGuiRenderer>();
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            g_renderer->initialize(w, h);
            g_renderer->setup_imgui(window);
            
            // Initialize UI
            g_esp_ui = std::make_shared<ESPUI>(g_renderer);
            
            // Initialize entity manager
            g_entity_manager = std::make_shared<EntityManager>(g_game_memory, g_renderer);
            
            g_initialized = true;
            printf("[Hooks] Initialization complete.\n");
        }
        
        // Render loop
        if (g_initialized) {
            // Update entity state
            g_entity_manager->update();
            
            // Begin ImGui frame
            g_renderer->begin_frame();
            
            // Update ESP UI (handling delta time)
            static auto last_time = std::chrono::steady_clock::now();
            auto current_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            g_esp_ui->update(delta_time);
            
            // Render ESP
            auto players = g_entity_manager->get_entities();
            g_esp_ui->render_esp(players);
            
            // Render Menu (if open)
            g_esp_ui->render_menu();
            
            // End ImGui frame
            g_renderer->end_frame();
        }
        
        // Call the original function to swap buffers
        original_SDL_GL_SwapWindow(window);
    }
    
    // Hook SDL_PollEvent to handle ImGui input
    typedef int (*SDL_PollEvent_t)(SDL_Event*);
    int SDL_PollEvent(SDL_Event* event) {
        static SDL_PollEvent_t original_SDL_PollEvent = nullptr;
        if (!original_SDL_PollEvent) {
            original_SDL_PollEvent = (SDL_PollEvent_t)dlsym(RTLD_NEXT, "SDL_PollEvent");
        }
        
        if (!original_SDL_PollEvent) return 0; // Should not happen
        
        int result = original_SDL_PollEvent(event);
        
        if (g_initialized && result && event) {
            // Let ImGui handle the event
            g_renderer->handle_event(event);
            
            // Toggle menu with Insert key
            if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_INSERT) {
                g_esp_ui->toggle_ui();
            }
        }
        
        return result;
    }
}

namespace Hooks {
    bool initialize() {
        // Since we are using LD_PRELOAD, we don't need to do much here.
        // The symbols are already exported and will be used by the dynamic linker.
        return true;
    }
    
    void shutdown() {
        // Cleanup if needed
    }
}
