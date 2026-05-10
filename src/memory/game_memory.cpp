#include "memory/game_memory.hpp"
#include "game/game_structures.hpp"
#include <algorithm>
#include <cmath>

GameMemory::GameMemory(MemoryReaderPtr reader) 
    : memory_reader(reader) {
}

std::vector<PlayerEntity> GameMemory::read_players() {
    std::vector<PlayerEntity> players;
    
    // Try to read up to 64 player entities
    for (uint32_t i = 0; i < 64; ++i) {
        uintptr_t entity_ptr = get_entity_from_list(i);
        if (entity_ptr == 0) continue;
        
        // Check if entity is valid
        uint32_t health = read_player_health(entity_ptr);
        if (health == 0) continue;
        
        PlayerEntity player = read_player_entity(entity_ptr, i);
        if (player.is_alive()) {
            player.bounding_box = calculate_aabb(entity_ptr);
            players.push_back(player);
        }
    }
    
    return players;
}

PlayerEntity GameMemory::read_local_player() {
    uintptr_t local_player_ptr = memory_reader->read<uintptr_t>(
        offsets::LOCAL_PLAYER
    );
    
    if (local_player_ptr == 0) {
        return PlayerEntity();
    }
    
    return read_player_entity(local_player_ptr, 0);
}

Vector3 GameMemory::read_player_position(uintptr_t entity_ptr) {
    if (entity_ptr == 0) return Vector3();
    
    return memory_reader->read_vector3(entity_ptr + offsets::PLAYER_POSITION);
}

uint32_t GameMemory::read_player_health(uintptr_t entity_ptr) {
    if (entity_ptr == 0) return 0;
    
    return memory_reader->read<uint32_t>(entity_ptr + offsets::PLAYER_HEALTH);
}

uint32_t GameMemory::read_player_team(uintptr_t entity_ptr) {
    if (entity_ptr == 0) return 0;
    
    return memory_reader->read<uint32_t>(entity_ptr + offsets::PLAYER_TEAM);
}

AABB GameMemory::calculate_aabb(uintptr_t entity_ptr) {
    if (entity_ptr == 0) return AABB();
    
    // Read bone matrices to calculate bounds
    std::vector<Vector3> bone_positions;
    
    // Read key bones for AABB calculation
    const uint32_t bones[] = {
        offsets::BONE_HEAD,
        offsets::BONE_NECK,
        offsets::BONE_CHEST,
        offsets::BONE_PELVIS,
        offsets::BONE_LEFT_FOOT,
        offsets::BONE_RIGHT_FOOT
    };
    
    for (uint32_t bone : bones) {
        Matrix4x4 bone_matrix = read_bone_matrix(entity_ptr, bone);
        bone_positions.push_back(bone_matrix.get_position());
    }
    
    if (bone_positions.empty()) {
        // Fallback: use player position with default size
        Vector3 pos = read_player_position(entity_ptr);
        return AABB(
            Vector3(pos.x - 15.0f, pos.y - 35.0f, pos.z - 15.0f),
            Vector3(pos.x + 15.0f, pos.y + 0.0f, pos.z + 15.0f)
        );
    }
    
    // Calculate bounds from bone positions
    Vector3 min = bone_positions[0];
    Vector3 max = bone_positions[0];
    
    for (const auto& pos : bone_positions) {
        min.x = std::min(min.x, pos.x);
        min.y = std::min(min.y, pos.y);
        min.z = std::min(min.z, pos.z);
        
        max.x = std::max(max.x, pos.x);
        max.y = std::max(max.y, pos.y);
        max.z = std::max(max.z, pos.z);
    }
    
    // Add padding
    const float padding = 10.0f;
    min.x -= padding;
    min.y -= padding;
    min.z -= padding;
    max.x += padding;
    max.y += padding;
    max.z += padding;
    
    return AABB(min, max);
}

Matrix4x4 GameMemory::read_bone_matrix(uintptr_t entity_ptr, uint32_t bone_index) {
    if (entity_ptr == 0) return Matrix4x4();
    
    uintptr_t bone_matrix_ptr = entity_ptr + offsets::PLAYER_BONE_MATRIX;
    uintptr_t bone_addr = bone_matrix_ptr + (bone_index * sizeof(Matrix4x4));
    
    return memory_reader->read_matrix(bone_addr);
}

PlayerEntity GameMemory::read_player_entity(uintptr_t entity_ptr, uint32_t entity_id) {
    PlayerEntity player;
    player.entity_id = entity_id;
    player.health = read_player_health(entity_ptr);
    player.max_health = 100; // CS2 default max health
    player.team = read_player_team(entity_ptr);
    player.position = read_player_position(entity_ptr);
    
    return player;
}

uintptr_t GameMemory::get_entity_from_list(uint32_t index) {
    // Simplified: in real implementation, would navigate entity list structure
    // This is pseudo-code for accessing entity list
    uintptr_t entity_list = memory_reader->read<uintptr_t>(offsets::ENTITY_LIST);
    if (entity_list == 0) return 0;
    
    // Entity list typically stores pointers in array or linked list
    // For this example, assume linear array of pointers
    uintptr_t entity_ptr = memory_reader->read<uintptr_t>(
        entity_list + (index * sizeof(uintptr_t))
    );
    
    return entity_ptr;
}
