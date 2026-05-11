#include "memory/game_memory.hpp"
#include "memory/offset_manager.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

GameMemory::GameMemory(MemoryReaderPtr reader) 
    : memory_reader(reader) {
    // In Linux, we need the base address of client_client.so
    auto linux_reader = std::dynamic_pointer_cast<LinuxMemoryReader>(reader);
    if (linux_reader) {
        client_base = linux_reader->get_module_base("client_client.so");
        fprintf(stdout, "[GameMemory] client_client.so base: 0x%lx\n", client_base);
    }
}

std::vector<PlayerEntity> GameMemory::read_players() {
    std::vector<PlayerEntity> players;
    
    if (client_base == 0) return players;

    const auto& offsets = OffsetManager::instance().get();
    uintptr_t entity_list_addr = client_base + offsets.entity_list;
    
    // In CS:GO Legacy Linux 64-bit, the entity list is often a bit more complex,
    // but for simplicity we'll try the direct access first.
    
    for (uint32_t i = 0; i < 64; ++i) {
        uintptr_t entity_ptr = get_entity_from_list(i);
        if (entity_ptr == 0) continue;
        
        uint32_t health = read_player_health(entity_ptr);
        if (health == 0 || health > 100) continue;
        
        PlayerEntity player = read_player_entity(entity_ptr, i);
        if (player.is_alive()) {
            player.bounding_box = calculate_aabb(entity_ptr);
            players.push_back(player);
        }
    }
    
    return players;
}

PlayerEntity GameMemory::read_local_player() {
    const auto& offsets = OffsetManager::instance().get();
    uintptr_t local_player_ptr = memory_reader->read<uintptr_t>(
        client_base + offsets.local_player
    );
    
    if (local_player_ptr == 0) return PlayerEntity();
    
    return read_player_entity(local_player_ptr, 0);
}

Vector3 GameMemory::read_player_position(uintptr_t entity_ptr) {
    if (entity_ptr == 0) return Vector3();
    const auto& offsets = OffsetManager::instance().get();
    return memory_reader->read_vector3(entity_ptr + offsets.m_vecOrigin);
}

uint32_t GameMemory::read_player_health(uintptr_t entity_ptr) {
    if (entity_ptr == 0) return 0;
    const auto& offsets = OffsetManager::instance().get();
    return memory_reader->read<uint32_t>(entity_ptr + offsets.m_iHealth);
}

uint32_t GameMemory::read_player_team(uintptr_t entity_ptr) {
    if (entity_ptr == 0) return 0;
    const auto& offsets = OffsetManager::instance().get();
    return memory_reader->read<uint32_t>(entity_ptr + offsets.m_iTeamNum);
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
    
    const auto& offsets = OffsetManager::instance().get();
    uintptr_t bone_matrix_ptr = memory_reader->read<uintptr_t>(entity_ptr + offsets.m_dwBoneMatrix);
    
    if (bone_matrix_ptr == 0) return Matrix4x4();
    
    // Each bone is represented by a 3x4 matrix in memory.
    // Struct size is 3 * 4 * sizeof(float) = 48 bytes (0x30).
    uintptr_t bone_addr = bone_matrix_ptr + (bone_index * 0x30);
    
    // Read the 3x4 matrix
    struct Matrix3x4 {
        float m[3][4];
    } mat3x4;
    
    memory_reader->read_memory(bone_addr, &mat3x4, sizeof(Matrix3x4));
    
    // Convert to our Matrix4x4 format for position extraction
    Matrix4x4 result;
    // Row 0
    result.data[0] = mat3x4.m[0][0]; result.data[1] = mat3x4.m[0][1]; result.data[2] = mat3x4.m[0][2]; result.data[3] = mat3x4.m[0][3];
    // Row 1
    result.data[4] = mat3x4.m[1][0]; result.data[5] = mat3x4.m[1][1]; result.data[6] = mat3x4.m[1][2]; result.data[7] = mat3x4.m[1][3];
    // Row 2
    result.data[8] = mat3x4.m[2][0]; result.data[9] = mat3x4.m[2][1]; result.data[10] = mat3x4.m[2][2]; result.data[11] = mat3x4.m[2][3];
    // Row 3 (Identity)
    result.data[12] = 0; result.data[13] = 0; result.data[14] = 0; result.data[15] = 1.0f;
    
    // In our Matrix4x4 struct, get_position() reads data[12], data[13], data[14].
    // Let's adapt result so get_position() works correctly with 3x4 logic.
    // Actually our Matrix4x4::get_position() returns data[12], data[13], data[14].
    // Wait, the standard Matrix3x4 translation is in m[0][3], m[1][3], m[2][3].
    // Let's store it where our Matrix4x4 expects the translation.
    result.data[12] = mat3x4.m[0][3]; // x
    result.data[13] = mat3x4.m[1][3]; // y
    result.data[14] = mat3x4.m[2][3]; // z
    
    return result;
}

PlayerEntity GameMemory::read_player_entity(uintptr_t entity_ptr, uint32_t entity_id) {
    PlayerEntity player;
    player.entity_id = entity_id;
    player.health = read_player_health(entity_ptr);
    player.max_health = 100; // CS:GO default max health
    player.team = read_player_team(entity_ptr);
    player.position = read_player_position(entity_ptr);
    
    return player;
}

uintptr_t GameMemory::get_entity_from_list(uint32_t index) {
    if (client_base == 0) return 0;
    const auto& offsets = OffsetManager::instance().get();
    uintptr_t entity_list = client_base + offsets.entity_list;
    if (entity_list == 0) return 0;
    
    // CS:GO Legacy entity list is an array of pointers separated by 0x10 bytes
    uintptr_t entity_ptr = memory_reader->read<uintptr_t>(
        entity_list + (index * 0x10)
    );
    
    if (index < 3) {
        static int calls = 0;
        if (calls < 5) {
            fprintf(stdout, "[DEBUG] get_entity_from_list(%u): reading from 0x%lx, got 0x%lx\n", 
                    index, entity_list + (index * 0x10), entity_ptr);
            calls++;
        }
    }
    
    return entity_ptr;
}
