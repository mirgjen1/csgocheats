#pragma once

#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>

/**
 * Game memory offsets for CS2
 * Source: https://github.com/frk1/hazedumper
 * Last updated: May 2026
 * 
 * Note: These are community-maintained offsets and may change with game updates.
 * If offsets stop working, check the hazedumper repository for updates.
 */
namespace offsets {
<<<<<<< HEAD
    // Verified CS:GO Legacy (Source 1) Linux 64-bit offsets
    // Note: These are relative to their respective modules (client_client.so)
=======
    // Entity list offsets (from hazedumper - updated 2023-09-05)
    constexpr uintptr_t ENTITY_LIST = 0x4E0102C;      // dwEntityList
    constexpr uintptr_t LOCAL_PLAYER = 0xDEB99C;      // dwLocalPlayer
>>>>>>> d03621e9466965c376953a8b4e61791aa78af68d
    
    // Static Netvars (These rarely change in Legacy)
    constexpr uintptr_t PLAYER_POSITION = 0x138;      // m_vecOrigin
    constexpr uintptr_t PLAYER_HEALTH = 0x100;        // m_iHealth
    constexpr uintptr_t PLAYER_TEAM = 0xF4;           // m_iTeamNum
<<<<<<< HEAD
=======
    constexpr uintptr_t PLAYER_NAME = 0x304C;         // m_szCustomName
>>>>>>> d03621e9466965c376953a8b4e61791aa78af68d
    constexpr uintptr_t PLAYER_BONE_MATRIX = 0x26A8;  // m_dwBoneMatrix
    
    // Bone indices for CS:GO Legacy
    constexpr uint32_t BONE_HEAD = 8;
    constexpr uint32_t BONE_NECK = 7;
    constexpr uint32_t BONE_CHEST = 6;
    constexpr uint32_t BONE_PELVIS = 0;
    constexpr uint32_t BONE_LEFT_FOOT = 73;
    constexpr uint32_t BONE_RIGHT_FOOT = 80;
    
    // dwLocalPlayer
    constexpr const char* LOCAL_PLAYER_SIG = "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 0B";
    
    // dwEntityList
    constexpr const char* ENTITY_LIST_SIG = "48 8D 05 ?? ?? ?? ?? 48 8B 00 48 85 C0 74 ?? 48 8B 00";
    
    // dwViewMatrix
    constexpr const char* VIEW_MATRIX_SIG = "48 8D 05 ?? ?? ?? ?? 48 8D 3D ?? ?? ?? ?? 48 8D 35";
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
 * Screen space rectangle for 2D rendering
 */
struct Rect2D {
    float x, y, width, height;
    
    Rect2D() : x(0), y(0), width(0), height(0) {}
    Rect2D(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
};

/**
 * Color structure (RGBA)
 */
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(255), g(255), b(255), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}
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
    
    bool is_alive() const { return health > 0; }
    float health_percentage() const {
        return max_health > 0 ? (static_cast<float>(health) / max_health) : 0.0f;
    }
};

/**
 * Transform matrix (4x4)
 */
struct Matrix4x4 {
    float data[16];
    
    Matrix4x4() { memset(data, 0, sizeof(data)); }
    
    Vector3 get_position() const {
        return Vector3(data[12], data[13], data[14]);
    }
};
