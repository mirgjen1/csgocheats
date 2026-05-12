#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <glm/glm.hpp>

/**
 * Game memory offsets for CS:GO Legacy (Source 1) Linux 64-bit
 */
namespace offsets {
    // Static fallback offsets (used if signature scanning fails)
    constexpr uintptr_t ENTITY_LIST = 0x2238468;
    constexpr uintptr_t LOCAL_PLAYER = 0x222a7f8;
    constexpr uintptr_t VIEW_MATRIX = 0x2213e40;
    
    // Netvars
    constexpr uintptr_t m_vecOrigin = 0x138;
    constexpr uintptr_t m_iHealth = 0x100;
    constexpr uintptr_t m_iTeamNum = 0xF4;
    constexpr uintptr_t m_szCustomName = 0x304C;
    constexpr uintptr_t m_dwBoneMatrix = 0x26A8;
    
    // Bone indices for CS:GO Legacy
    constexpr uint32_t BONE_HEAD = 8;
    constexpr uint32_t BONE_NECK = 7;
    constexpr uint32_t BONE_CHEST = 6;
    constexpr uint32_t BONE_PELVIS = 0;
    constexpr uint32_t BONE_LEFT_FOOT = 73;
    constexpr uint32_t BONE_RIGHT_FOOT = 80;
    
    // Signature patterns for dynamic offset detection
    constexpr const char* LOCAL_PLAYER_SIG = "48 8b 05 ? ? ? ? 48 85 c0 74 0b 48 8b 00";
    constexpr const char* ENTITY_LIST_SIG = "48 8b 05 ? ? ? ? 48 8d 14 d0 48 8b 02";
    constexpr const char* VIEW_MATRIX_SIG = "48 8d 05 ? ? ? ? 48 8b 00 48 85 c0 74 08";
}

/**
 * 3D Vector structure
 */
struct Vector3 {
    float x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3 center() const { return *this; }
};

/**
 * 2D Vector for screen coordinates
 */
struct Vector2 {
    float x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

/**
 * Axis-Aligned Bounding Box
 */
struct AABB {
    Vector3 min;
    Vector3 max;
    
    AABB() = default;
    AABB(Vector3 min, Vector3 max) : min(min), max(max) {}
    
    Vector3 center() const {
        return Vector3(
            (min.x + max.x) / 2.0f,
            (min.y + max.y) / 2.0f,
            (min.z + max.z) / 2.0f
        );
    }
};

/**
 * Player entity structure
 */
struct PlayerEntity {
    uint32_t entity_id;
    uint32_t health;
    uint32_t max_health;
    uint32_t team;
    Vector3 position;
    AABB bounding_box;
    
    bool is_alive() const { return health > 0 && health <= 100; }
};

/**
 * Screen space rectangle for 2D rendering
 */
struct Rect2D {
    float x, y;
    float width, height;
    
    Rect2D() : x(0), y(0), width(0), height(0) {}
    Rect2D(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
};

/**
 * Color structure (RGBA)
 */
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(255), g(255), b(255), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
};

/**
 * Matrix 4x4 for view-projection and bone matrices
 */
struct Matrix4x4 {
    float data[16];
    
    Vector3 get_position() const {
        // Translation is stored in data[12], data[13], data[14] per game_memory.cpp
        return Vector3(data[12], data[13], data[14]);
    }
};
