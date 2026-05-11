#pragma once

#include <vector>
#include <memory>
#include "memory_reader.hpp"
#include "game/game_structures.hpp"

/**
 * Game memory interface for reading CS2-specific data
 */
class GameMemory {
public:
    explicit GameMemory(MemoryReaderPtr reader);
    
    /**
     * Update and retrieve all player entities
     */
    std::vector<PlayerEntity> read_players();
    
    /**
     * Get local player data
     */
    PlayerEntity read_local_player();
    
    /**
     * Read player position from memory
     */
    Vector3 read_player_position(uintptr_t entity_ptr);
    
    /**
     * Read player health
     */
    uint32_t read_player_health(uintptr_t entity_ptr);
    
    /**
     * Read player team (2 = Terrorist, 3 = Counter-Terrorist)
     */
    uint32_t read_player_team(uintptr_t entity_ptr);
    
    /**
     * Calculate AABB from bone positions
     */
    AABB calculate_aabb(uintptr_t entity_ptr);
    
    /**
     * Read bone matrix for skeletal calculations
     */
    Matrix4x4 read_bone_matrix(uintptr_t entity_ptr, uint32_t bone_index);
    
private:
    MemoryReaderPtr memory_reader;
    
    /**
     * Read a single player entity
     */
    PlayerEntity read_player_entity(uintptr_t entity_ptr, uint32_t entity_id);
    
    /**
     * Get entity pointer from entity list
     */
    uintptr_t get_entity_from_list(uint32_t index);
    
    uintptr_t client_base = 0;
};

using GameMemoryPtr = std::shared_ptr<GameMemory>;
