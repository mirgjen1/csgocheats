#include "memory/game_memory.hpp"
#include "memory/offset_manager.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

GameMemory::GameMemory(MemoryReaderPtr reader) 
    : memory_reader(reader) {
    // In Linux, we need the base address of the client module
    auto linux_reader = std::dynamic_pointer_cast<LinuxMemoryReader>(reader);
    if (linux_reader) {
        std::string mod_name = OffsetManager::instance().get_client_module_name();
        client_base = linux_reader->get_module_base(mod_name.c_str());
        fprintf(stdout, "[GameMemory] %s base: 0x%lx\n", mod_name.c_str(), client_base);
    }
}

std::vector<PlayerEntity> GameMemory::read_players() {
    std::vector<PlayerEntity> players;
    
    if (client_base == 0) return players;

    const auto& offsets = OffsetManager::instance().get();
    
    // Only scan 32 players to save CPU during debugging
    for (uint32_t i = 0; i < 32; ++i) {
        uintptr_t entity_ptr = get_entity_from_list(i);
        
        // CRITICAL: Validate pointer before reading.
        // Game pointers on 64-bit Linux are usually above 0x100000000000
        if (entity_ptr < 0x700000000000 || entity_ptr > 0x7fffffffffff) continue;
        
        uint32_t health = read_player_health(entity_ptr);
        if (health == 0 || health > 100) continue;
        
        PlayerEntity player = read_player_entity(entity_ptr, i);
        if (player.is_alive()) {
            // Simplified box to prevent lag from reading too many bones
            Vector3 pos = player.position;
            player.bounding_box = AABB(
                Vector3(pos.x - 15.0f, pos.y - 15.0f, pos.z - 35.0f),
                Vector3(pos.x + 15.0f, pos.y + 15.0f, pos.z + 35.0f)
            );
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
    
    // CS:GO Legacy / CS2 on Linux 64-bit uses 0x20 stride
    uintptr_t entity_ptr = memory_reader->read<uintptr_t>(
        entity_list + (index * 0x20)
    );
    
    if (entity_ptr != 0) {
        // Basic sanity check: pointer should be in a reasonable range
        if (entity_ptr < 0x1000 || entity_ptr > 0x7FFFFFFFFFFF) {
            return 0;
        }
        
        static int debug_count = 0;
        if (debug_count < 3) {
            fprintf(stdout, "[DEBUG] Entity[%u] found at 0x%lx (from 0x%lx)\n", 
                    index, entity_ptr, entity_list + (index * 0x20));
            debug_count++;
        }
    }
    
    return entity_ptr;
}

Matrix4x4 GameMemory::read_view_matrix() {
    Matrix4x4 matrix;
    if (client_base == 0) return matrix;
    
    const auto& offsets = OffsetManager::instance().get();
    if (offsets.view_matrix == 0) return matrix;
    
    uintptr_t vm_addr = client_base + offsets.view_matrix;
    memory_reader->read_memory(vm_addr, matrix.data, sizeof(matrix.data));
    
    return matrix;
}
