#pragma once

#include "rendering/renderer.hpp"
#include <string>
#include <vector>

/**
 * UI Backend for high-level overlay elements
 */
class UIBackend {
public:
    UIBackend(RendererPtr renderer);
    
    /**
     * Draw the main watermark/logo
     */
    void draw_watermark();
    
    /**
     * Draw status information (FPS, entity count, etc.)
     */
    void draw_status_panel(int fps, size_t entity_count);
    
    /**
     * Draw a simple notification message
     */
    void show_notification(const std::string& message, float duration_sec = 3.0f);
    
    /**
     * Update animations and timers
     */
    void update(float delta_time);
    
    /**
     * Draw all active UI elements
     */
    void render();

private:
    RendererPtr renderer;
    
    struct Notification {
        std::string message;
        float remaining_time;
    };
    
    std::vector<Notification> notifications;
};

using UIBackendPtr = std::shared_ptr<UIBackend>;
