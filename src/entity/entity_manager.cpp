#include "entity/entity_manager.hpp"
#include "rendering/visualizer.hpp"
#include <chrono>

EntityManager::EntityManager(GameMemoryPtr game_memory, RendererPtr renderer)
    : game_memory(game_memory), renderer(renderer) {
    last_update = std::chrono::steady_clock::now();
}

void EntityManager::update() {
    if (!should_update()) return;
    
    // Read all players from game memory
    entities = game_memory->read_players();
}

void EntityManager::render() {
    if (!renderer || !renderer->is_ready()) return;
    
    for (const auto& entity : entities) {
        // Skip based on configuration
        if (!entity.is_alive()) continue;
        if (config.render_enemies_only && entity.team == 3) continue; // Skip if CT and enemies_only
        
        // Draw player
        PlayerVisualizer::draw_player(
            renderer,
            entity,
            false,
            config.render_team_color
        );
    }
}

void EntityManager::set_config(const Config& new_config) {
    config = new_config;
}

const std::vector<PlayerEntity>& EntityManager::get_entities() const {
    return entities;
}

size_t EntityManager::entity_count() const {
    return entities.size();
}

void EntityManager::clear() {
    entities.clear();
}

bool EntityManager::should_update() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_update
    ).count();
    
    if (elapsed >= static_cast<long long>(config.update_interval_ms)) {
        last_update = now;
        return true;
    }
    
    return false;
}
