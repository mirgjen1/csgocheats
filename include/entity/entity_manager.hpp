#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include "game/game_structures.hpp"
#include "memory/game_memory.hpp"
#include "rendering/renderer.hpp"

/**
 * Entity manager for tracking and rendering players
 */
class EntityManager {
public:
    struct Config {
        bool render_aabb = true;
        bool render_health_bar = true;
        bool render_team_color = true;
        bool render_local_player = true;
        bool render_enemies_only = false;
        float update_interval_ms = 16.0f; // ~60 FPS
    };
    
    explicit EntityManager(GameMemoryPtr game_memory, RendererPtr renderer);
    
    /**
     * Update entity list from game memory
     */
    void update();
    
    /**
     * Render all entities
     */
    void render();
    
    /**
     * Set configuration
     */
    void set_config(const Config& config);
    
    /**
     * Get current entities
     */
    const std::vector<PlayerEntity>& get_entities() const;
    
    /**
     * Get entity count
     */
    size_t entity_count() const;
    
    /**
     * Clear all entities
     */
    void clear();
    
private:
    GameMemoryPtr game_memory;
    RendererPtr renderer;
    std::vector<PlayerEntity> entities;
    Config config;
    
    std::chrono::steady_clock::time_point last_update;
    
    /**
     * Check if enough time has passed for update
     */
    bool should_update();
};

using EntityManagerPtr = std::shared_ptr<EntityManager>;
