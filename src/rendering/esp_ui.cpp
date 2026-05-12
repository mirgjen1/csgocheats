#include "rendering/esp_ui.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

// ============================================================================
// ESPConfig Implementation
// ============================================================================

bool ESPConfig::save_to_file(const char* filename) const {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "[ESPUI] Failed to save config to %s\n", filename);
        return false;
    }
    
    fprintf(file, "[ESP_CONFIG]\n");
    fprintf(file, "enabled=%d\n", enabled);
    fprintf(file, "draw_boxes=%d\n", draw_boxes);
    fprintf(file, "draw_health_bars=%d\n", draw_health_bars);
    fprintf(file, "draw_player_names=%d\n", draw_player_names);
    fprintf(file, "draw_team_colors=%d\n", draw_team_colors);
    fprintf(file, "draw_skeletons=%d\n", draw_skeletons);
    fprintf(file, "draw_snaplines=%d\n", draw_snaplines);
    fprintf(file, "draw_distance=%d\n", draw_distance);
    fprintf(file, "render_enemies_only=%d\n", render_enemies_only);
    fprintf(file, "render_visible_only=%d\n", render_visible_only);
    fprintf(file, "render_local_player=%d\n", render_local_player);
    fprintf(file, "box_thickness=%.2f\n", box_thickness);
    fprintf(file, "line_thickness=%.2f\n", line_thickness);
    fprintf(file, "snapline_thickness=%.2f\n", snapline_thickness);
    fprintf(file, "max_render_distance=%.2f\n", max_render_distance);
    fprintf(file, "debug_mode=%d\n", debug_mode);
    fprintf(file, "show_entity_count=%d\n", show_entity_count);
    fprintf(file, "show_fps=%d\n", show_fps);
    
    fclose(file);
    fprintf(stdout, "[ESPUI] Config saved to %s\n", filename);
    return true;
}

bool ESPConfig::load_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "[ESPUI] Config file not found: %s (using defaults)\n", filename);
        return false;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '[' || line[0] == '\0') continue;
        
        // Simple key=value parsing
        char key[128], value_str[128];
        if (sscanf(line, "%127[^=]=%127s", key, value_str) != 2) continue;
        
        int value_int = atoi(value_str);
        float value_float = atof(value_str);
        
        if (strcmp(key, "enabled") == 0) enabled = value_int;
        else if (strcmp(key, "draw_boxes") == 0) draw_boxes = value_int;
        else if (strcmp(key, "draw_health_bars") == 0) draw_health_bars = value_int;
        else if (strcmp(key, "draw_player_names") == 0) draw_player_names = value_int;
        else if (strcmp(key, "draw_team_colors") == 0) draw_team_colors = value_int;
        else if (strcmp(key, "draw_skeletons") == 0) draw_skeletons = value_int;
        else if (strcmp(key, "draw_snaplines") == 0) draw_snaplines = value_int;
        else if (strcmp(key, "draw_distance") == 0) draw_distance = value_int;
        else if (strcmp(key, "render_enemies_only") == 0) render_enemies_only = value_int;
        else if (strcmp(key, "render_visible_only") == 0) render_visible_only = value_int;
        else if (strcmp(key, "render_local_player") == 0) render_local_player = value_int;
        else if (strcmp(key, "box_thickness") == 0) box_thickness = value_float;
        else if (strcmp(key, "line_thickness") == 0) line_thickness = value_float;
        else if (strcmp(key, "snapline_thickness") == 0) snapline_thickness = value_float;
        else if (strcmp(key, "max_render_distance") == 0) max_render_distance = value_float;
        else if (strcmp(key, "debug_mode") == 0) debug_mode = value_int;
        else if (strcmp(key, "show_entity_count") == 0) show_entity_count = value_int;
        else if (strcmp(key, "show_fps") == 0) show_fps = value_int;
    }
    
    fclose(file);
    fprintf(stdout, "[ESPUI] Config loaded from %s\n", filename);
    return true;
}

// ============================================================================
// ESPUI Implementation
// ============================================================================

ESPUI::ESPUI(RendererPtr renderer) : renderer(renderer) {
    // Load config from file if it exists
    config.load_from_file("/tmp/csgocheat_esp_config.txt");
}

void ESPUI::update(float delta_time) {
    frame_time = delta_time;
    
    // Animate menu visibility
    if (menu_open) {
        menu_animation = std::min(1.0f, menu_animation + delta_time * 5.0f);
    } else {
        menu_animation = std::max(0.0f, menu_animation - delta_time * 5.0f);
    }
}

void ESPUI::render_esp(const std::vector<PlayerEntity>& entities) {
    if (!config.enabled || !renderer) return;
    
    for (const auto& entity : entities) {
        if (should_render_entity(entity)) {
            render_single_entity(entity);
        }
    }
}

void ESPUI::render_single_entity(const PlayerEntity& entity) {
    Color color = get_entity_color(entity);
    
    // Draw main AABB box
    if (config.draw_boxes) {
        render_entity_box(entity, color);
    }
    
    // Draw health bar
    if (config.draw_health_bars) {
        render_entity_health_bar(entity, color);
    }
    
    // Draw player name
    if (config.draw_player_names) {
        render_entity_name(entity, color);
    }
    
    // Draw snapline from center to player
    if (config.draw_snaplines) {
        render_snapline(entity, color);
    }
    
    // Draw distance text
    if (config.draw_distance) {
        render_distance_text(entity);
    }
    
    // Draw skeleton
    if (config.draw_skeletons) {
        render_skeleton(entity, color);
    }
}

void ESPUI::render_entity_box(const PlayerEntity& entity, const Color& color) {
    if (!renderer) return;
    
    // Draw the 3D AABB with black outline
    renderer->draw_aabb(entity.bounding_box, config.box_outline_color, config.box_thickness + 1.0f);
    
    // Draw colored box (slightly thinner)
    renderer->draw_aabb(entity.bounding_box, color, config.box_thickness);
}

void ESPUI::render_entity_health_bar(const PlayerEntity& entity, const Color& color) {
    if (!renderer) return;
    
    // Project center of bounding box to screen
    Vector3 world_center = entity.bounding_box.center();
    Vector2 screen_pos;
    
    if (!renderer->world_to_screen(world_center, screen_pos)) {
        return; // Not on screen
    }
    
    // Offset health bar above the entity
    screen_pos.x -= 25.0f; // Half bar width
    screen_pos.y -= 50.0f;
    
    // Calculate health bar width based on health percentage
    float health_percentage = entity.health / static_cast<float>(entity.max_health);
    health_percentage = std::max(0.0f, std::min(1.0f, health_percentage));
    
    // Draw background (dark)
    Rect2D bg(screen_pos.x, screen_pos.y, 50.0f, 5.0f);
    renderer->draw_filled_rect(bg, Color(0, 0, 0, 200));
    
    // Draw health bar
    float bar_width = 50.0f * health_percentage;
    Rect2D health_bar(screen_pos.x, screen_pos.y, bar_width, 5.0f);
    
    // Color gradient: green (100%) -> yellow (50%) -> red (0%)
    Color health_color;
    if (health_percentage > 0.5f) {
        // Green to yellow
        uint8_t r = static_cast<uint8_t>(255 * (health_percentage - 0.5f) * 2.0f);
        health_color = Color(r, 255, 0, 255);
    } else {
        // Yellow to red
        uint8_t g = static_cast<uint8_t>(255 * health_percentage * 2.0f);
        health_color = Color(255, g, 0, 255);
    }
    
    renderer->draw_filled_rect(health_bar, health_color);
    
    // Draw border
    renderer->draw_box_2d(bg, Color(255, 255, 255, 150), 1.0f);
}

void ESPUI::render_entity_name(const PlayerEntity& entity, const Color& color) {
    if (!renderer) return;
    
    // Project center to screen
    Vector3 world_center = entity.bounding_box.center();
    Vector2 screen_pos;
    
    if (!renderer->world_to_screen(world_center, screen_pos)) {
        return;
    }
    
    char name_text[64];
    snprintf(name_text, sizeof(name_text), "Player %u (%s)",
             entity.entity_id,
             entity.team == 2 ? "T" : entity.team == 3 ? "CT" : "?");
    
    renderer->draw_text(screen_pos, name_text, color);
}

void ESPUI::render_snapline(const PlayerEntity& entity, const Color& color) {
    if (!renderer) return;
    
    // Get entity screen position
    Vector3 entity_center = entity.bounding_box.center();
    Vector2 entity_screen;
    
    if (!renderer->world_to_screen(entity_center, entity_screen)) {
        return; // Not on screen
    }
    
    // Draw line from center of screen to entity
    Vector2 screen_center(renderer->get_width() / 2.0f, renderer->get_height() / 2.0f);
    renderer->draw_line(screen_center, entity_screen, color, config.snapline_thickness);
}

void ESPUI::render_distance_text(const PlayerEntity& entity) {
    if (!renderer) return;
    
    float distance = get_entity_distance(entity);
    
    // Project center to screen
    Vector3 world_center = entity.bounding_box.center();
    Vector2 screen_pos;
    
    if (!renderer->world_to_screen(world_center, screen_pos)) {
        return;
    }
    
    // Offset below the name
    screen_pos.y += 12.0f;
    
    char distance_text[32];
    snprintf(distance_text, sizeof(distance_text), "%.0f units", distance);
    renderer->draw_text(screen_pos, distance_text, Color(200, 200, 200, 255));
}

void ESPUI::render_skeleton(const PlayerEntity& entity, const Color& color) {
    if (!renderer) return;
    
    // TODO: Implement skeleton rendering from bone matrices
    // For now, this is a placeholder
}

void ESPUI::render_menu() {
    if (!renderer || menu_animation <= 0.0f) return;
    
    // Apply animation to menu position
    int menu_x = 10 + static_cast<int>(-150 * (1.0f - menu_animation));
    
    // Semi-transparent background
    Rect2D menu_bg(menu_x, 10, 200, 300);
    renderer->draw_filled_rect(menu_bg, Color(20, 20, 20, 180));
    renderer->draw_box_2d(menu_bg, Color(100, 150, 255, 255), 2.0f);
    
    // Title
    renderer->draw_text(Vector2(menu_x + 10, 15), "ESP SETTINGS", Color(100, 150, 255, 255));
    
    // Menu items
    int y_offset = 40;
    const char* menu_items[] = {
        config.draw_boxes ? "[✓] Draw Boxes" : "[ ] Draw Boxes",
        config.draw_health_bars ? "[✓] Health Bars" : "[ ] Health Bars",
        config.draw_player_names ? "[✓] Player Names" : "[ ] Player Names",
        config.draw_snaplines ? "[✓] Snaplines" : "[ ] Snaplines",
        config.draw_distance ? "[✓] Distance" : "[ ] Distance",
        config.render_enemies_only ? "[✓] Enemies Only" : "[ ] Enemies Only",
        "Save Config",
        "Load Config",
        "Reset Defaults",
        "Close Menu"
    };
    
    const int menu_item_count = sizeof(menu_items) / sizeof(menu_items[0]);
    
    for (int i = 0; i < menu_item_count; ++i) {
        Color text_color = (i == selected_menu_item) ? Color(255, 200, 0, 255) : Color(200, 200, 200, 255);
        
        if (i == selected_menu_item) {
            Rect2D highlight(menu_x + 5, 30 + y_offset + i * 20 - 2, 190, 18);
            renderer->draw_filled_rect(highlight, Color(100, 100, 100, 100));
        }
        
        renderer->draw_text(Vector2(menu_x + 10, 35 + y_offset + i * 20), menu_items[i], text_color);
    }
}

void ESPUI::render_debug(size_t entity_count, float fps) {
    if (!config.debug_mode || !renderer) return;
    
    int debug_x = 10;
    int debug_y = renderer->get_height() - 100;
    
    // Semi-transparent background for debug info
    Rect2D debug_bg(debug_x - 5, debug_y - 5, 200, 90);
    renderer->draw_filled_rect(debug_bg, Color(0, 0, 0, 150));
    renderer->draw_box_2d(debug_bg, Color(100, 255, 100, 200), 1.0f);
    
    // Debug text
    char debug_text[256];
    
    if (config.show_entity_count) {
        snprintf(debug_text, sizeof(debug_text), "Entities: %zu", entity_count);
        renderer->draw_text(Vector2(debug_x, debug_y), debug_text, Color(100, 255, 100, 255));
    }
    
    if (config.show_fps) {
        snprintf(debug_text, sizeof(debug_text), "FPS: %.1f", fps);
        renderer->draw_text(Vector2(debug_x, debug_y + 15), debug_text, Color(100, 255, 100, 255));
    }
    
    snprintf(debug_text, sizeof(debug_text), "UI State: %d", static_cast<int>(current_state));
    renderer->draw_text(Vector2(debug_x, debug_y + 30), debug_text, Color(100, 255, 100, 255));
}

void ESPUI::toggle_ui() {
    menu_open = !menu_open;
}

void ESPUI::set_ui_state(UIState state) {
    current_state = state;
}

ESPUI::UIState ESPUI::get_ui_state() const {
    return current_state;
}

ESPConfig& ESPUI::get_config() {
    return config;
}

const ESPConfig& ESPUI::get_config() const {
    return config;
}

void ESPUI::set_config(const ESPConfig& cfg) {
    config = cfg;
}

void ESPUI::reset_to_defaults() {
    config = ESPConfig();
    fprintf(stdout, "[ESPUI] Reset to default configuration\n");
}

bool ESPUI::should_render_entity(const PlayerEntity& entity) const {
    if (!entity.is_alive()) return false;
    
    // Filter by enemies only
    if (config.render_enemies_only && entity.team != 2 && entity.team != 3) return false;
    
    // Filter by render distance
    float distance = get_entity_distance(entity);
    if (distance > config.max_render_distance) return false;
    
    return true;
}

Color ESPUI::get_entity_color(const PlayerEntity& entity) const {
    if (config.draw_team_colors) {
        return entity.team == 2 ? config.enemy_color : config.team_color;
    }
    return Color(255, 255, 255, 255); // White if no team color
}

float ESPUI::get_entity_distance(const PlayerEntity& entity) const {
    // Distance from origin (0, 0, 0) - can be enhanced with local player position
    Vector3 entity_pos = entity.bounding_box.center();
    return std::sqrt(entity_pos.x * entity_pos.x +
                     entity_pos.y * entity_pos.y +
                     entity_pos.z * entity_pos.z);
}
