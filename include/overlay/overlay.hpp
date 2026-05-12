#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include "memory/game_memory.hpp"
#include "entity/entity_manager.hpp"
#include "rendering/renderer.hpp"

#ifdef _WIN32
class OverlayWindow;
using OverlayWindowPtr = std::shared_ptr<OverlayWindow>;
#endif

/**
 * Main overlay application class
 */
class Overlay {
public:
    struct Config {
        uint32_t window_width = 1920;
        uint32_t window_height = 1080;
        const char* window_title = "CS2 Overlay";
        const char* process_name = "cs2";
        EntityManager::Config entity_config;
    };
    
    Overlay() = default;
    ~Overlay();
    
    /**
     * Initialize overlay system
     */
    bool initialize(const Config& config);
    
    /**
     * Main overlay loop - call from main thread
     */
    void run();
    
    /**
     * Stop overlay
     */
    void stop();
    
    /**
     * Check if overlay is running
     */
    bool is_running() const;
    
    /**
     * Get entity manager for configuration
     */
    EntityManagerPtr get_entity_manager();
    
    /**
     * Set custom render callback for additional elements
     */
    using RenderCallback = std::function<void(RendererPtr)>;
    void set_render_callback(RenderCallback callback);
    
    /**
     * Get renderer instance
     */
    RendererPtr get_renderer() { return renderer; }
    
private:
    Config config;
    MemoryReaderPtr memory_reader;
    GameMemoryPtr game_memory;
    RendererPtr renderer;
    EntityManagerPtr entity_manager;
    
#ifdef _WIN32
    OverlayWindowPtr overlay_win;
#endif
    
    std::atomic<bool> running{false};
    RenderCallback custom_render;
    
    /**
     * Render frame
     */
    void render_frame();
};

using OverlayPtr = std::shared_ptr<Overlay>;
