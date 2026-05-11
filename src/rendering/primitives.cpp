#include "rendering/visualizer.hpp"
#include <algorithm>

/**
 * HealthBarVisualizer implementation
 */

void HealthBarVisualizer::draw_health_bar(
    RendererPtr renderer,
    const Vector2& screen_pos,
    uint32_t health,
    uint32_t max_health) {
    
    if (!renderer || max_health == 0) return;
    
    float health_percentage = static_cast<float>(health) / max_health;
    Color health_color = get_health_color(health_percentage);
    
    // Draw background (dark red)
    Rect2D background(screen_pos.x, screen_pos.y, BAR_WIDTH, BAR_HEIGHT);
    renderer->draw_filled_rect(background, Color(50, 0, 0, 200));
    
    // Draw health bar (green to red gradient)
    float bar_width = BAR_WIDTH * health_percentage;
    Rect2D health_bar(screen_pos.x, screen_pos.y, bar_width, BAR_HEIGHT);
    renderer->draw_filled_rect(health_bar, health_color);
    
    // Draw border
    Rect2D border(screen_pos.x, screen_pos.y, BAR_WIDTH, BAR_HEIGHT);
    renderer->draw_box_2d(border, Color(255, 255, 255, 150), 1.0f);
}

void HealthBarVisualizer::draw_armor_bar(
    RendererPtr renderer,
    const Vector2& screen_pos,
    uint32_t armor,
    uint32_t max_armor) {
    
    if (!renderer || max_armor == 0) return;
    
    float armor_percentage = static_cast<float>(armor) / max_armor;
    
    // Draw background (dark blue)
    Rect2D background(screen_pos.x, screen_pos.y - BAR_HEIGHT - 2.0f, BAR_WIDTH, BAR_HEIGHT);
    renderer->draw_filled_rect(background, Color(0, 0, 50, 200));
    
    // Draw armor bar (blue)
    float bar_width = BAR_WIDTH * armor_percentage;
    Rect2D armor_bar(screen_pos.x, screen_pos.y - BAR_HEIGHT - 2.0f, bar_width, BAR_HEIGHT);
    renderer->draw_filled_rect(armor_bar, Color(0, 100, 255, 255));
    
    // Draw border
    Rect2D border(screen_pos.x, screen_pos.y - BAR_HEIGHT - 2.0f, BAR_WIDTH, BAR_HEIGHT);
    renderer->draw_box_2d(border, Color(255, 255, 255, 150), 1.0f);
}

Color HealthBarVisualizer::get_health_color(float health_percentage) {
    // Green (100%) -> Yellow (50%) -> Red (0%)
    if (health_percentage > 0.5f) {
        // Green to yellow
        uint8_t r = static_cast<uint8_t>(255 * (health_percentage - 0.5f) * 2.0f);
        uint8_t g = 255;
        return Color(r, g, 0, 255);
    } else {
        // Yellow to red
        uint8_t r = 255;
        uint8_t g = static_cast<uint8_t>(255 * health_percentage * 2.0f);
        return Color(r, g, 0, 255);
    }
}

/**
 * AABBVisualizer implementation
 */

void AABBVisualizer::draw_3d_aabb(
    RendererPtr renderer,
    const AABB& aabb,
    const Color& color,
    float thickness) {
    
    if (!renderer) return;
    renderer->draw_aabb(aabb, color, thickness);
}

void AABBVisualizer::draw_skeleton(
    RendererPtr renderer,
    const std::vector<Vector3>& bone_positions,
    const Color& color,
    float thickness) {
    
    if (!renderer || bone_positions.size() < 2) return;
    
    // Draw lines connecting bones
    // This is simplified - real implementation would use proper skeletal connections
    for (size_t i = 1; i < bone_positions.size(); ++i) {
        Vector2 from_screen, to_screen;
        
        // Project to screen space
        if (renderer->world_to_screen(bone_positions[i-1], from_screen) &&
            renderer->world_to_screen(bone_positions[i], to_screen)) {
            renderer->draw_line(from_screen, to_screen, color, thickness);
        }
    }
}

/**
 * PlayerVisualizer implementation
 */

void PlayerVisualizer::draw_player(
    RendererPtr renderer,
    const PlayerEntity& player,
    bool local_player,
    bool draw_name) {
    
    if (!renderer || !player.is_alive()) return;
    
    // Choose color based on team or local player status
    Color box_color = local_player ? get_local_player_color() : get_team_color(player.team);
    
    // Draw AABB box
    AABBVisualizer::draw_3d_aabb(renderer, player.bounding_box, box_color, 2.0f);
    
    // Draw health bar above player
    Vector3 world_center = player.bounding_box.center();
    Vector2 health_bar_pos;
    if (!renderer->world_to_screen(world_center, health_bar_pos)) {
        return; // Player not on screen
    }
    health_bar_pos.x -= HealthBarVisualizer::BAR_WIDTH / 2.0f;
    health_bar_pos.y -= 20.0f; // Offset above the box
    
    HealthBarVisualizer::draw_health_bar(
        renderer,
        health_bar_pos,
        player.health,
        player.max_health
    );
    
    // Optional: Draw team color indicator
    if (!local_player) {
        Vector2 corner_pos = health_bar_pos;
        corner_pos.x -= TEAM_COLOR_OFFSET;
        renderer->draw_circle(corner_pos, 3.0f, box_color);
    }
}

Color PlayerVisualizer::get_team_color(uint32_t team) {
    switch (team) {
        case 2:  // Terrorist (T)
            return Color(200, 100, 0, 255);  // Orange
        case 3:  // Counter-Terrorist (CT)
            return Color(100, 150, 255, 255); // Light Blue
        default:
            return Color(255, 255, 255, 255); // White
    }
}

Color PlayerVisualizer::get_local_player_color() {
    return Color(0, 255, 0, 255); // Bright green
}
