# CS2 Overlay - Implementation Guide

## Quick Start

### 1. Project Setup

```bash
# Clone/extract project
cd ~/Documents/csgocheats

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build project
cmake --build . --config Release

# Run
./cs2overlay
```

### 2. Key Implementation Areas

## Memory Reading Implementation

### Finding Correct Offsets

The offsets defined in `include/game/game_structures.hpp` are **examples**. Here's how to find real offsets:

#### Step 1: Identify Player Structure
```cpp
// Use Cheat Engine to find offset chains
1. Search for exact health value (e.g., 100)
2. Hit in game to reduce health (e.g., 87)
3. Search for new value
4. Filter down to single address
5. Get pointer chain: game.exe -> offset1 -> offset2 -> health
```

#### Step 2: Map to Entity List
```cpp
// Find entity pool/array
1. Find local player address
2. Scan for arrays of pointers to players
3. Validate with health readings at +0x324 offset
4. Calculate entity list base and stride
```

#### Step 3: Record Offsets
```cpp
// Update game_structures.hpp with correct values
constexpr uintptr_t ENTITY_LIST = 0xYOUR_VALUE;
constexpr uintptr_t LOCAL_PLAYER = 0xYOUR_VALUE;
constexpr uintptr_t PLAYER_POSITION = 0xYOUR_VALUE;
// ... etc
```

### Example Offset Discovery Script

```cpp
// test_memory_offsets.cpp - Diagnostic utility
#include "memory/memory_reader.hpp"
#include <iostream>

void scan_player_data(MemoryReaderPtr reader) {
    // Read local player
    uintptr_t local = reader->read<uintptr_t>(0x1A8A6D8);
    std::cout << "Local player: 0x" << std::hex << local << std::endl;
    
    // Try different offsets for health
    for (int offset = 0x300; offset < 0x400; offset += 4) {
        uint32_t value = reader->read<uint32_t>(local + offset);
        if (value > 0 && value <= 100) {
            std::cout << "Possible health at +0x" << std::hex << offset 
                      << " = " << std::dec << value << std::endl;
        }
    }
    
    // Try different offsets for position (check for floats in reasonable range)
    for (int offset = 0x1000; offset < 0x1400; offset += 4) {
        Vector3 pos = reader->read<Vector3>(local + offset);
        if (pos.x > -10000 && pos.x < 10000 &&  // Reasonable map bounds
            pos.y > -10000 && pos.y < 10000 &&
            pos.z > -10000 && pos.z < 10000) {
            std::cout << "Possible position at +0x" << std::hex << offset 
                      << " = (" << pos.x << ", " << pos.y << ", " << pos.z << ")" 
                      << std::endl;
        }
    }
}
```

## Rendering Implementation

### DirectX 11 Backend Example

```cpp
// rendering/dx11_renderer.hpp (Pseudocode)

#include <d3d11.h>
#include "rendering/renderer.hpp"

class DirectX11Renderer : public Renderer {
private:
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11Buffer* constant_buffer = nullptr;
    
public:
    bool initialize(uint32_t width, uint32_t height) override;
    void draw_line(const Vector2& from, const Vector2& to, 
                   const Color& color, float thickness = 1.0f) override;
    // ... implement other drawing functions
};
```

### OpenGL Backend Example

```cpp
// rendering/opengl_renderer.hpp (Pseudocode)

#include <GL/gl.h>
#include "rendering/renderer.hpp"

class OpenGLRenderer : public Renderer {
private:
    GLuint vao, vbo, ebo;
    GLuint shader_program;
    
public:
    bool initialize(uint32_t width, uint32_t height) override;
    void draw_line(const Vector2& from, const Vector2& to, 
                   const Color& color, float thickness = 1.0f) override;
    // ... implement other drawing functions
};
```

## Advanced Techniques

### 1. Bone-Based AABB

Instead of static bounding box, calculate dynamic AABB from bone positions:

```cpp
// More accurate, follows player animation
AABB calculate_dynamic_aabb(uintptr_t entity_ptr) {
    // Read bone transform matrices
    std::vector<Vector3> bones;
    for (uint32_t i = 0; i < 25; ++i) {  // 25 bones in CS2
        Matrix4x4 matrix = read_bone_matrix(entity_ptr, i);
        if (matrix.is_valid()) {
            bones.push_back(matrix.get_position());
        }
    }
    
    // Calculate convex hull or OBB for even more accuracy
    // For now, use axis-aligned approach
    return calculate_bounds_from_points(bones);
}
```

### 2. Screen-Space Rendering

Some elements render better in 2D screen space:

```cpp
void render_player_info(RendererPtr renderer, const PlayerEntity& player) {
    // Convert player position to screen coordinates once
    Vector2 screen_pos;
    if (!renderer->world_to_screen(player.position, screen_pos)) {
        return;  // Off-screen
    }
    
    // Draw all 2D elements relative to this screen position
    float info_width = 100.0f;
    float info_height = 60.0f;
    
    // Panel
    Rect2D panel(screen_pos.x - info_width/2, screen_pos.y - info_height,
                 info_width, info_height);
    renderer->draw_filled_rect(panel, Color(0, 0, 0, 200));
    renderer->draw_box_2d(panel, Color(255, 255, 255, 255), 1.0f);
    
    // Health text
    std::string health_text = "HP: " + std::to_string(player.health);
    renderer->draw_text(screen_pos, health_text.c_str(), Color(0, 255, 0));
    
    // Distance (if camera matrix available)
    // float distance = calculate_distance_to_local(player);
}
```

### 3. Culling & Optimization

```cpp
// Only render visible/relevant players
bool should_render_player(const PlayerEntity& player) {
    // Skip if dead
    if (!player.is_alive()) return false;
    
    // Skip if too far away
    const float MAX_RENDER_DISTANCE = 5000.0f;
    if (calculate_distance(player.position) > MAX_RENDER_DISTANCE) {
        return false;
    }
    
    // Skip if off-screen after projection
    Vector2 screen_pos;
    if (!world_to_screen(player.position, screen_pos)) {
        return false;
    }
    
    // Add small margin (e.g., 100px off-screen) for partial rendering
    if (screen_pos.x < -100 || screen_pos.x > width + 100 ||
        screen_pos.y < -100 || screen_pos.y > height + 100) {
        return false;
    }
    
    return true;
}

// Render with culling
void render_optimized() {
    for (const auto& player : entities) {
        if (should_render_player(player)) {
            PlayerVisualizer::draw_player(renderer, player);
        }
    }
}
```

### 4. Multi-threaded Memory Reading

```cpp
// Prevent frame rate drops from memory reads
class ThreadedGameMemory {
private:
    std::thread reader_thread;
    std::mutex entity_lock;
    std::vector<PlayerEntity> cached_entities;
    std::atomic<bool> running{true};
    
public:
    void update_thread_worker() {
        while (running) {
            auto entities = memory->read_players();
            {
                std::lock_guard<std::mutex> lock(entity_lock);
                cached_entities = entities;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(33));  // 30 FPS
        }
    }
    
    std::vector<PlayerEntity> get_entities() {
        std::lock_guard<std::mutex> lock(entity_lock);
        return cached_entities;
    }
};
```

## Testing & Validation

### 1. Unit Testing Template

```cpp
// tests/test_memory.cpp
#include <gtest/gtest.h>
#include "memory/memory_reader.hpp"

class MockMemoryTest : public ::testing::Test {
protected:
    MockMemoryReader reader;
};

TEST_F(MockMemoryTest, ReadVector3) {
    Vector3 expected(1.0f, 2.0f, 3.0f);
    // Write to mock memory
    // reader.write(0x1000, &expected, sizeof(Vector3));
    
    // Read back
    Vector3 result = reader.read<Vector3>(0x1000);
    
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(MockMemoryTest, AABBCalculation) {
    AABB box(Vector3(0, 0, 0), Vector3(10, 20, 30));
    EXPECT_EQ(box.center().x, 5.0f);
    EXPECT_EQ(box.center().y, 10.0f);
    EXPECT_EQ(box.center().z, 15.0f);
}
```

### 2. Visual Debugging

```cpp
// Render diagnostic information
void render_debug_info() {
    if (!debug_mode) return;
    
    // Draw entity list status
    renderer->draw_text(Vector2(10, 10), 
        "Entities: " + std::to_string(entity_manager->entity_count()),
        Color(255, 255, 0));
    
    // Draw memory read status
    renderer->draw_text(Vector2(10, 30), 
        "Memory: OK",  // or error message
        Color(0, 255, 0));
    
    // Draw FPS counter
    renderer->draw_text(Vector2(10, 50), 
        "FPS: " + std::to_string(calculate_fps()),
        Color(255, 255, 255));
    
    // Draw camera matrix info
    renderer->draw_text(Vector2(10, 70), 
        "Camera Valid: " + std::string(camera_valid ? "YES" : "NO"),
        Color(camera_valid ? 0x00FF00 : 0xFF0000));
}
```

### 3. Performance Profiling

```cpp
#include <chrono>

struct FrameMetrics {
    std::chrono::milliseconds memory_time;
    std::chrono::milliseconds render_time;
    std::chrono::milliseconds total_time;
};

FrameMetrics measure_frame_performance() {
    auto start = std::chrono::high_resolution_clock::now();
    
    auto mem_start = std::chrono::high_resolution_clock::now();
    entity_manager->update();
    auto mem_end = std::chrono::high_resolution_clock::now();
    
    auto render_start = std::chrono::high_resolution_clock::now();
    entity_manager->render();
    auto render_end = std::chrono::high_resolution_clock::now();
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return {
        std::chrono::duration_cast<std::chrono::milliseconds>(mem_end - mem_start),
        std::chrono::duration_cast<std::chrono::milliseconds>(render_end - render_start),
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
    };
}
```

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| No players rendered | Wrong entity list offset | Use Cheat Engine to find correct offset |
| Bounding boxes offset from players | Incorrect position offset | Verify PLAYER_POSITION offset |
| Health bar shows wrong values | Health offset incorrect or max_health assumption wrong | Scan for health values during gameplay |
| Overlay doesn't appear | Rendering backend not initialized | Check renderer->is_ready() return value |
| Memory read fails on update | Process terminated or page unmapped | Add error handling and retry logic |
| High CPU usage | Update interval too low or large entity count | Increase update_interval_ms or reduce entity limit |

## Next Steps

1. **Find Correct Offsets** - Use Cheat Engine to locate CS2 offsets
2. **Implement Renderer** - Choose rendering backend and implement primitives
3. **Test Memory Reading** - Verify player data is read correctly
4. **Optimize Performance** - Profile and optimize hot paths
5. **Add Features** - Skeleton rendering, distance calculations, etc.

---

For more information, see [FRAMEWORK_DESIGN.md](FRAMEWORK_DESIGN.md)
