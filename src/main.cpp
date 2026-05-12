#include <iostream>
#include <memory>
#include <thread>
#include "overlay/overlay.hpp"
#include "rendering/ui_backend.hpp"

/**
 * Example usage of the CS2 overlay system
 */

int main() {
    std::cout << "=== CS:GO Legacy Overlay Framework ===" << std::endl;
    std::cout << "Initializing overlay system..." << std::endl;
    
    // Create overlay with configuration
    auto overlay = std::make_shared<Overlay>();
    
    Overlay::Config config;
    config.window_width = 1920;
    config.window_height = 1080;
    config.window_title = "CS:GO Legacy Overlay";
    config.process_name = "csgo_linux64"; // CS:GO Legacy process name on Linux
    
    // Configure entity rendering
    config.entity_config.render_aabb = true;
    config.entity_config.render_health_bar = true;
    config.entity_config.render_team_color = true;
    config.entity_config.render_local_player = true;
    config.entity_config.render_enemies_only = false;
    config.entity_config.update_interval_ms = 16.0f; // 60 FPS
    
    if (!overlay->initialize(config)) {
        std::cerr << "Failed to initialize overlay! (Is CS:GO Legacy running?)" << std::endl;
        return 1;
    }
    
    std::cout << "Overlay initialized successfully!" << std::endl;
    std::cout << "Resolution: " << config.window_width << "x" << config.window_height << std::endl;
    // Create UI backend
    auto ui_backend = std::make_shared<UIBackend>(overlay->get_renderer());
    auto entity_manager = overlay->get_entity_manager();
    
    // Set up custom render callback for additional UI elements
    overlay->set_render_callback([ui_backend, entity_manager](RendererPtr renderer) {
        // Update UI state (mocking delta time)
        ui_backend->update(0.016f);
        
        // Draw UI elements
        ui_backend->draw_status_panel(60, entity_manager->entity_count());
        ui_backend->render();
        
        // Draw crosshair
        Vector2 center(renderer->get_width() / 2.0f, renderer->get_height() / 2.0f);
        renderer->draw_circle(center, 5.0f, Color(0, 255, 0, 255));
    });
    
    // Run overlay in background thread
    std::thread overlay_thread([overlay]() {
        overlay->run();
    });
    
    // Monitoring will be done via the captured entity_manager in the callback or via overlay status
    
    std::cout << "\nOverlay is running!" << std::endl;
    std::cout << "Press any key to stop..." << std::endl;
    
    // Monitor entity updates in main thread
    int frame_count = 0;
    while (overlay->is_running()) {
        // Monitor entities - slow down the logic loop slightly to save CPU
        entity_manager->update();
        
        if (frame_count % 100 == 0) {
            std::cout << "\rFrame: " << frame_count << " | Entities: " << entity_manager->entity_count() << std::flush;
        }
        frame_count++;
        
        // Increase sleep to 32ms (approx 30 updates per second) to prevent game lag
        std::this_thread::sleep_for(std::chrono::milliseconds(32));
    }
    
    std::cout << "\nShutting down overlay..." << std::endl;
    
    if (overlay_thread.joinable()) {
        overlay_thread.join();
    }
    
    std::cout << "Overlay stopped successfully!" << std::endl;
    return 0;
}
