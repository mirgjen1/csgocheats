/**
 * CS:GO Overlay ESP UI Usage Example
 * 
 * This example demonstrates how to configure and use the ESP UI system
 * in the CS:GO overlay.
 */

#include "overlay/overlay.hpp"
#include "rendering/esp_ui.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // =====================================================================
    // 1. Create and Initialize Overlay
    // =====================================================================
    
    Overlay overlay;
    
    Overlay::Config config;
    config.window_width = 1920;
    config.window_height = 1080;
    config.window_title = "CS:GO Overlay with ESP";
    config.process_name = "csgo_linux64"; // Target process
    
    if (!overlay.initialize(config)) {
        std::cerr << "Failed to initialize overlay" << std::endl;
        return 1;
    }
    
    // =====================================================================
    // 2. Get ESP UI Instance and Configure
    // =====================================================================
    
    ESPUIPtr esp_ui = overlay.get_esp_ui();
    ESPConfig& esp_config = esp_ui->get_config();
    
    // Enable all ESP features
    esp_config.enabled = true;
    esp_config.draw_boxes = true;
    esp_config.draw_health_bars = true;
    esp_config.draw_player_names = true;
    esp_config.draw_team_colors = true;
    
    // Optional: Enable advanced features
    esp_config.draw_snaplines = false;      // Lines from screen center
    esp_config.draw_distance = false;       // Distance text
    esp_config.draw_skeletons = false;      // Bone visualization
    
    // Filtering options
    esp_config.render_enemies_only = false; // Show all players
    esp_config.max_render_distance = 8000.0f;
    
    // Customization
    esp_config.box_thickness = 2.0f;
    esp_config.line_thickness = 1.5f;
    
    // Debug mode (shows FPS and entity count)
    esp_config.debug_mode = true;
    esp_config.show_entity_count = true;
    esp_config.show_fps = true;
    
    std::cout << "[ESP] Configured overlay with ESP system enabled" << std::endl;
    
    // =====================================================================
    // 3. Customize Colors (Optional)
    // =====================================================================
    
    // Team colors
    esp_config.enemy_color = {200, 100, 0, 255};      // Orange for T
    esp_config.team_color = {100, 150, 255, 255};     // Blue for CT
    esp_config.local_player_color = {0, 255, 0, 255}; // Green for self
    esp_config.box_outline_color = {0, 0, 0, 255};    // Black outline
    
    // =====================================================================
    // 4. Load or Save Configuration
    // =====================================================================
    
    // Load from file if exists
    bool loaded = esp_config.load_from_file("/tmp/csgocheat_esp_config.txt");
    if (!loaded) {
        std::cout << "[ESP] No saved config found, using defaults" << std::endl;
    }
    
    // Save current configuration
    esp_config.save_to_file("/tmp/csgocheat_esp_config.txt");
    std::cout << "[ESP] Config saved to /tmp/csgocheat_esp_config.txt" << std::endl;
    
    // =====================================================================
    // 5. Setup Custom Render Callback (Optional)
    // =====================================================================
    
    overlay.set_render_callback([](RendererPtr renderer) {
        // Custom rendering on top of ESP
        // Example: Draw a crosshair at screen center
        if (renderer) {
            Vector2 center(renderer->get_width() / 2.0f, 
                          renderer->get_height() / 2.0f);
            renderer->draw_circle(center, 5.0f, {255, 255, 0, 255});
        }
    });
    
    std::cout << "[ESP] Overlay initialized and ready!" << std::endl;
    std::cout << "[ESP] Features enabled:" << std::endl;
    std::cout << "  - AABB Boxes: " << (esp_config.draw_boxes ? "ON" : "OFF") << std::endl;
    std::cout << "  - Health Bars: " << (esp_config.draw_health_bars ? "ON" : "OFF") << std::endl;
    std::cout << "  - Player Names: " << (esp_config.draw_player_names ? "ON" : "OFF") << std::endl;
    std::cout << "  - Debug Mode: " << (esp_config.debug_mode ? "ON" : "OFF") << std::endl;
    
    std::cout << "\n[ESP] Running overlay..." << std::endl;
    std::cout << "[ESP] Press Ctrl+C to stop\n" << std::endl;
    
    // =====================================================================
    // 6. Run Main Overlay Loop
    // =====================================================================
    
    overlay.run();
    
    // =====================================================================
    // 7. Cleanup
    // =====================================================================
    
    std::cout << "[ESP] Overlay shutting down..." << std::endl;
    
    return 0;
}

/**
 * RUNTIME CONFIGURATION EXAMPLES
 * 
 * Once the overlay is running, you can modify ESP settings dynamically:
 * 
 * // In a real application, you might have keybinds or UI menus:
 * 
 * void on_key_press(int key) {
 *     ESPUIPtr esp_ui = overlay->get_esp_ui();
 *     ESPConfig& config = esp_ui->get_config();
 *     
 *     switch(key) {
 *         case KEY_E:  // Toggle ESP
 *             config.enabled = !config.enabled;
 *             break;
 *         case KEY_B:  // Toggle boxes
 *             config.draw_boxes = !config.draw_boxes;
 *             break;
 *         case KEY_H:  // Toggle health bars
 *             config.draw_health_bars = !config.draw_health_bars;
 *             break;
 *         case KEY_D:  // Toggle distance
 *             config.draw_distance = !config.draw_distance;
 *             break;
 *         case KEY_S:  // Toggle snaplines
 *             config.draw_snaplines = !config.draw_snaplines;
 *             break;
 *         case KEY_V:  // Toggle enemies only
 *             config.render_enemies_only = !config.render_enemies_only;
 *             break;
 *         case KEY_R:  // Reset to defaults
 *             esp_ui->reset_to_defaults();
 *             break;
 *         case KEY_L:  // Load saved config
 *             config.load_from_file("/tmp/csgocheat_esp_config.txt");
 *             break;
 *         case KEY_C:  // Save current config
 *             config.save_to_file("/tmp/csgocheat_esp_config.txt");
 *             break;
 *     }
 * }
 * 
 * ACCESSING ENTITY INFORMATION
 * 
 * // Get detected entities from entity manager:
 * EntityManagerPtr entity_mgr = overlay->get_entity_manager();
 * const auto& entities = entity_mgr->get_entities();
 * 
 * for (const auto& player : entities) {
 *     std::cout << "Player " << player.entity_id 
 *               << " HP: " << player.health 
 *               << " Team: " << player.team << std::endl;
 * }
 * 
 * DEBUG OUTPUT
 * 
 * When debug_mode is enabled, the overlay displays:
 * - Entity count (bottom-left)
 * - FPS counter
 * - Current UI state
 * 
 * PERFORMANCE OPTIMIZATION
 * 
 * For better performance on lower-end systems:
 * 
 * config.draw_snaplines = false;    // Disables line rendering
 * config.draw_skeletons = false;    // Disables bone visualization
 * config.box_thickness = 1.0f;      // Thinner boxes
 * config.max_render_distance = 5000.0f;  // Shorter render distance
 * config.render_enemies_only = true;     // Half the entity rendering
 * 
 * ADVANCED: CUSTOM ESP ELEMENTS
 * 
 * To add custom ESP elements, use the render callback:
 * 
 * overlay.set_render_callback([esp_ui](RendererPtr renderer) {
 *     if (!renderer) return;
 *     
 *     const auto& entities = esp_ui->get_entities();
 *     for (const auto& entity : entities) {
 *         // Custom rendering per entity
 *         Vector3 head_pos = entity.bounding_box.max;  // Head position
 *         Vector2 screen_pos;
 *         
 *         if (renderer->world_to_screen(head_pos, screen_pos)) {
 *             // Draw custom element at head position
 *             renderer->draw_text(screen_pos, "HEAD", {255, 0, 0, 255});
 *         }
 *     }
 * });
 */
