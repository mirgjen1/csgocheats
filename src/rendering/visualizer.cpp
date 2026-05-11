#include "rendering/visualizer.hpp"
#include <algorithm>
#include <cstdio>

void HealthBarVisualizer::draw_health_bar(
    RendererPtr renderer,
    const Vector2& screen_pos,
    uint32_t health,
    uint32_t max_health
) {
    if (max_health == 0) return;
    
    float percentage = std::clamp(static_cast<float>(health) / max_health, 0.0f, 1.0f);
    Color color = get_health_color(percentage);
    
    // Background (gray)
    renderer->draw_rect(screen_pos, Vector2(BAR_WIDTH, BAR_HEIGHT), Color(50, 50, 50, 200));
    
    // Foreground (health color)
    renderer->draw_rect(screen_pos, Vector2(BAR_WIDTH * percentage, BAR_HEIGHT), color);
}

Color HealthBarVisualizer::get_health_color(float health_percentage) {
    if (health_percentage > 0.5f) {
        // Green to Yellow
        float factor = (health_percentage - 0.5f) * 2.0f;
        return Color(
            static_cast<uint8_t>(255 * (1.0f - factor)),
            255,
            0,
            255
        );
    } else {
        // Yellow to Red
        float factor = health_percentage * 2.0f;
        return Color(
            255,
            static_cast<uint8_t>(255 * factor),
            0,
            255
        );
    }
}

void AABBVisualizer::draw_3d_aabb(
    RendererPtr renderer,
    const AABB& aabb,
    const Color& color,
    float thickness
) {
    // This requires a World-to-Screen projection function
    // For now, we'll implement a simple 2D box if we have screen coordinates
    // In a real implementation, we would project all 8 corners of the AABB
}

void PlayerVisualizer::draw_player(
    RendererPtr renderer,
    const PlayerEntity& player,
    bool local_player,
    bool draw_name
) {
    if (!renderer) return;

    Vector2 foot_pos, head_pos;
    // CS:GO player model is roughly 72 units tall
    Vector3 head_world_pos = player.position;
    head_world_pos.z += 72.0f;

    if (renderer->world_to_screen(player.position, foot_pos) && 
        renderer->world_to_screen(head_world_pos, head_pos)) {
        
        float height = foot_pos.y - head_pos.y;
        float width = height / 2.0f;
        
        Vector2 top_left(head_pos.x - width / 2.0f, head_pos.y);
        
        // Use team color or local player color
        Color color = local_player ? get_local_player_color() : get_team_color(player.team);
        
        // Draw 2D Box (ESP)
        Rect2D box = { top_left.x, top_left.y, width, height };
        renderer->draw_box_2d(box, color, 2.0f);
        
        // Draw Health Bar
        HealthBarVisualizer::draw_health_bar(
            renderer,
            Vector2(top_left.x, top_left.y - 8),
            player.health,
            player.max_health
        );
        
        // Draw HP text if needed
        if (draw_name) {
            char hp_text[16];
            snprintf(hp_text, sizeof(hp_text), "HP: %d", player.health);
            renderer->draw_text(Vector2(top_left.x + width + 5, top_left.y), hp_text, Color(255, 255, 255, 255));
        }
    }
}

Color PlayerVisualizer::get_local_player_color() {
    return Color(0, 255, 0, 255); // Green
}

Color PlayerVisualizer::get_team_color(uint32_t team) {
    if (team == 2) return Color(255, 100, 100, 255); // Terrorist (Red)
    if (team == 3) return Color(100, 100, 255, 255); // CT (Blue)
    return Color(255, 255, 255, 255);
}
