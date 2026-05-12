#pragma once

#include "game/game_structures.hpp"
#include "rendering/renderer.hpp"
#include <cstdint>

/**
 * ESP (Electronic Sight Potential) UI Configuration
 * Manages visualization settings for player detection and rendering
 */
class ESPConfig {
public:
    // Box rendering
    bool enabled = true;
    bool draw_boxes = true;
    bool draw_health_bars = true;
    bool draw_player_names = true;
    bool draw_team_colors = true;
    bool draw_skeletons = false;
    bool draw_snaplines = false;
    bool draw_distance = false;
    
    // Visibility checks
    bool render_enemies_only = false;
    bool render_visible_only = false;
    bool render_local_player = false;
    
    // Customization
    float box_thickness = 2.0f;
    float line_thickness = 1.5f;
    float snapline_thickness = 1.0f;
    
    // Colors (can be customized)
    Color enemy_color = Color(200, 100, 0, 255);      // Orange for T
    Color team_color = Color(100, 150, 255, 255);     // Light blue for CT
    Color local_player_color = Color(0, 255, 0, 255); // Green for self
    Color box_outline_color = Color(0, 0, 0, 255);    // Black outline
    
    // Distance-based rendering
    float max_render_distance = 8000.0f; // Units
    
    // Debug
    bool debug_mode = false;
    bool show_entity_count = true;
    bool show_fps = true;
    
    // Save/Load settings
    bool save_to_file(const char* filename) const;
    bool load_from_file(const char* filename);
};

/**
 * ESP UI System
 * Handles rendering of ESP overlays and UI elements
 */
class ESPUI {
public:
    enum class UIState {
        HIDDEN,      // No UI visible
        MENU,        // Settings menu open
        DEBUG,       // Debug info display
        FULL         // Full UI with menu + debug
    };
    
    explicit ESPUI(RendererPtr renderer);
    
    /**
     * Update ESP UI state (input handling, animations, etc.)
     */
    void update(float delta_time);
    
    /**
     * Render ESP overlays for detected entities
     */
    void render_esp(const std::vector<PlayerEntity>& entities);
    
    /**
     * Render UI menu/settings panel
     */
    void render_menu();
    
    /**
     * Render debug information
     */
    void render_debug(size_t entity_count, float fps);
    
    /**
     * Toggle UI visibility
     */
    void toggle_ui();
    
    /**
     * Set UI state
     */
    void set_ui_state(UIState state);
    
    /**
     * Get current UI state
     */
    UIState get_ui_state() const;
    
    /**
     * Get ESP configuration
     */
    ESPConfig& get_config();
    const ESPConfig& get_config() const;
    
    /**
     * Set ESP configuration
     */
    void set_config(const ESPConfig& cfg);
    
    /**
     * Reset to default configuration
     */
    void reset_to_defaults();
    
private:
    RendererPtr renderer;
    ESPConfig config;
    UIState current_state = UIState::HIDDEN;
    
    float menu_animation = 0.0f;
    float frame_time = 0.0f;
    
    // Menu state
    int selected_menu_item = 0;
    bool menu_open = false;
    
    // Helper functions
    void render_single_entity(const PlayerEntity& entity);
    void render_entity_box(const PlayerEntity& entity, const Color& color);
    void render_entity_health_bar(const PlayerEntity& entity, const Color& color);
    void render_entity_name(const PlayerEntity& entity, const Color& color);
    void render_snapline(const PlayerEntity& entity, const Color& color);
    void render_distance_text(const PlayerEntity& entity);
    void render_skeleton(const PlayerEntity& entity, const Color& color);
    
    // Menu rendering
    void render_menu_background();
    void render_menu_items();
    void render_color_picker(int item_index, Color& target_color, const char* label);
    
    // Utility
    bool should_render_entity(const PlayerEntity& entity) const;
    Color get_entity_color(const PlayerEntity& entity) const;
    float get_entity_distance(const PlayerEntity& entity) const;
};

using ESPUIPtr = std::shared_ptr<ESPUI>;
