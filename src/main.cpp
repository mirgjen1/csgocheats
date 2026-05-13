#include <iostream>
#include <memory>
#include <thread>
#include "overlay/overlay.hpp"
#include "rendering/esp_ui.hpp"

/**
 * CS:GO Legacy Overlay with ESP UI
 */

int main() {
    std::cout << "=== CS:GO Legacy Overlay Framework ===" << std::endl;
    std::cout << "Initializing overlay system..." << std::endl;
    
    // Create overlay with configuration
    auto overlay = std::make_shared<Overlay>();
    
    Overlay::Config config;
    config.window_width = 1920;
    config.window_height = 1080;
    config.window_title = "CS:GO Legacy Overlay - ESP";
    config.process_name = "csgo_linux64"; // CS:GO Legacy process name on Linux
    
    // Configure entity rendering
    config.entity_config.render_aabb = true;
    config.entity_config.render_health_bar = true;
    config.entity_config.render_team_color = true;
    config.entity_config.render_local_player = false;
    config.entity_config.render_enemies_only = false;
    config.entity_config.update_interval_ms = 16.0f; // 60 FPS
    
    if (!overlay->initialize(config)) {
        std::cerr << "Failed to initialize overlay! (Is CS:GO Legacy running?)" << std::endl;
        return 1;
    }
    
    std::cout << "Overlay initialized successfully!" << std::endl;
    std::cout << "Resolution: " << config.window_width << "x" << config.window_height << std::endl;
    
    // Get ESP UI and configure it
    auto esp_ui = overlay->get_esp_ui();
    auto entity_manager = overlay->get_entity_manager();
    auto renderer = overlay->get_renderer();
    
    // Configure ESP settings
    ESPConfig& esp_config = esp_ui->get_config();
    esp_config.enabled = true;
    esp_config.draw_boxes = true;
    esp_config.draw_health_bars = true;
    esp_config.draw_player_names = true;
    esp_config.draw_team_colors = true;
    esp_config.draw_snaplines = false;
    esp_config.draw_distance = false;
    esp_config.render_enemies_only = false;
    esp_config.debug_mode = true;
    esp_config.show_entity_count = true;
    esp_config.show_fps = true;
    
    std::cout << "\n[ESP] Configuration:" << std::endl;
    std::cout << "  - Boxes: " << (esp_config.draw_boxes ? "ON" : "OFF") << std::endl;
    std::cout << "  - Health Bars: " << (esp_config.draw_health_bars ? "ON" : "OFF") << std::endl;
    std::cout << "  - Player Names: " << (esp_config.draw_player_names ? "ON" : "OFF") << std::endl;
    std::cout << "  - Debug Mode: " << (esp_config.debug_mode ? "ON" : "OFF") << std::endl;
    
    // Run overlay in background thread
    std::thread overlay_thread([overlay]() {
        overlay->run();
    });
    
    std::cout << "\nOverlay is running!" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // Monitor entity updates in main thread
    int frame_count = 0;
    while (overlay->is_running()) {
        // Update entities
        entity_manager->update();
        
        // Update ESP UI
        esp_ui->update(0.016f);
        
        // Print debug info every 100 frames
        if (frame_count % 100 == 0) {
            size_t entity_count = entity_manager->entity_count();
            if (entity_count > 0) {
                std::cout << "\rFrame: " << frame_count << " | Entities: " << entity_count << std::flush;
            }
        }
        frame_count++;
        
        // Sleep to reduce CPU usage (target ~30 Hz for update loop)
        std::this_thread::sleep_for(std::chrono::milliseconds(32));
    }
    
    std::cout << "\nShutting down overlay..." << std::endl;
    
    if (overlay_thread.joinable()) {
        overlay_thread.join();
    }
    
    std::cout << "Overlay stopped successfully!" << std::endl;
    return 0;
}
