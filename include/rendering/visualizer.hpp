#pragma once

#include "game/game_structures.hpp"
#include "rendering/renderer.hpp"

/**
 * Visualizer for player health bars and entity information
 */
class HealthBarVisualizer {
public:
    static constexpr float BAR_WIDTH = 50.0f;
    static constexpr float BAR_HEIGHT = 5.0f;
    static constexpr float BAR_OFFSET_Y = -10.0f;
    
    /**
     * Draw health bar for a player entity
     * @param renderer Rendering interface
     * @param screen_pos Screen position for the bar (top-left)
     * @param health Current health
     * @param max_health Maximum health
     */
    static void draw_health_bar(
        RendererPtr renderer,
        const Vector2& screen_pos,
        uint32_t health,
        uint32_t max_health
    );
    
    /**
     * Draw armor bar
     */
    static void draw_armor_bar(
        RendererPtr renderer,
        const Vector2& screen_pos,
        uint32_t armor,
        uint32_t max_armor
    );
    
    /**
     * Get color based on health percentage
     */
    static Color get_health_color(float health_percentage);
};

/**
 * AABB visualization utilities
 */
class AABBVisualizer {
public:
    /**
     * Draw axis-aligned bounding box in 3D space
     * Projects to 2D screen coordinates
     */
    static void draw_3d_aabb(
        RendererPtr renderer,
        const AABB& aabb,
        const Color& color = Color(0, 255, 0, 255),
        float thickness = 2.0f
    );
    
    /**
     * Draw skeleton/bones from bone matrices
     */
    static void draw_skeleton(
        RendererPtr renderer,
        const std::vector<Vector3>& bone_positions,
        const Color& color = Color(0, 255, 0, 255),
        float thickness = 1.5f
    );
};

/**
 * Player entity visualization
 */
class PlayerVisualizer {
public:
    static constexpr float TEAM_COLOR_OFFSET = 5.0f;
    
    /**
     * Draw complete player visualization:
     * - AABB box
     * - Team color indicator
     * - Health bar
     * - Player name (optional)
     */
    static void draw_player(
        RendererPtr renderer,
        const PlayerEntity& player,
        bool local_player = false,
        bool draw_name = true
    );
    
    /**
     * Get color based on team
     */
    static Color get_team_color(uint32_t team);
    
    /**
     * Get color for local player
     */
    static Color get_local_player_color();
};
