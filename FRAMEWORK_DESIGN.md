# CS2 External Overlay Framework - Conceptual Documentation

## Overview

This is a conceptual C++ framework for building an external overlay for Counter-Strike 2. The project demonstrates key concepts for:

1. **Real-time game memory reading** - Accessing player data via process memory
2. **3D-to-2D projection** - Converting world coordinates to screen space
3. **AABB rendering** - Drawing bounding boxes around entities
4. **Dynamic UI elements** - Health bars and status indicators

## Project Structure

```
csgocheats/
├── CMakeLists.txt              # Build configuration
├── include/                    # Header files
│   ├── game/
│   │   └── game_structures.hpp # Game data structures and offsets
│   ├── memory/
│   │   ├── memory_reader.hpp   # Memory access interface
│   │   └── game_memory.hpp     # Game-specific memory reading
│   ├── rendering/
│   │   ├── renderer.hpp        # Rendering interface
│   │   └── visualizer.hpp      # UI visualization utilities
│   ├── entity/
│   │   └── entity_manager.hpp  # Entity lifecycle management
│   └── overlay/
│       └── overlay.hpp         # Main overlay application
└── src/                        # Implementation files
    ├── main.cpp
    ├── memory/
    ├── rendering/
    ├── entity/
    └── overlay/
```

## Core Components

### 1. Game Memory Management (`memory/`)

#### Purpose
Access game memory to retrieve player positions, health, team information, and bone matrices.

#### Key Classes
- **`MemoryReader`** (Abstract)
  - `read_memory()` - Raw memory reading interface
  - `read<T>()` - Templated type reading
  - `read_vector3()`, `read_matrix()` - Specialized data reading

- **Platform Implementations**
  - `WindowsMemoryReader` - Uses Windows API (`ReadProcessMemory`)
  - `LinuxMemoryReader` - Uses `/proc/[pid]/mem` interface
  - `MockMemoryReader` - For testing/demonstration

#### Game Memory Offsets
```
ENTITY_LIST         - 0x1A8C690   (Base address of player array)
LOCAL_PLAYER        - 0x1A8A6D8   (Pointer to local player entity)
PLAYER_POSITION     - 0x1344      (Vector3 position relative to entity)
PLAYER_HEALTH       - 0x324       (uint32_t health value)
PLAYER_TEAM         - 0x3C0       (uint32_t team: 2=T, 3=CT)
PLAYER_BONE_MATRIX  - 0x1080      (Transform matrices for skeletal animation)
```

**Note:** These offsets are examples and vary by game version. Real implementation requires reverse engineering.

### 2. Rendering System (`rendering/`)

#### Architecture
The rendering system uses an abstract interface pattern to support multiple rendering backends.

```
Renderer (Abstract Interface)
    ↓
OverlayRenderer (Screen-space overlay)
    ├── DirectX 11 (Windows)
    ├── OpenGL (Cross-platform)
    └── Vulkan (Advanced)
```

#### Rendering Primitives

**3D-to-2D Projection**
```cpp
bool world_to_screen(const Vector3& world_pos, Vector2& screen_pos) const
```
- Transforms 3D world coordinates to 2D screen coordinates
- Uses view and projection matrices
- Returns false if point is behind camera or off-screen

**Drawing Functions**
- `draw_aabb()` - Projects 3D bounding box corners and draws edges
- `draw_box_2d()` - Screen-space rectangle
- `draw_line()` - Line between two 2D points
- `draw_filled_rect()` - Filled rectangle
- `draw_circle()` - Circular outline
- `draw_text()` - Text rendering

#### Visualization Utilities

**`PlayerVisualizer`** - High-level player rendering
```cpp
draw_player(renderer, player, is_local, draw_name)
```
- Draws AABB box with team color
- Adds health bar above player head
- Optional team color indicator dot

**`HealthBarVisualizer`** - Health/armor status
```cpp
draw_health_bar(renderer, screen_pos, health, max_health)
draw_armor_bar(renderer, screen_pos, armor, max_armor)
```
- Dynamic color: Green (100%) → Yellow (50%) → Red (0%)
- Background frame with border
- Configurable width/height

**`AABBVisualizer`** - Bounding box rendering
```cpp
draw_3d_aabb(renderer, aabb, color, thickness)
draw_skeleton(renderer, bone_positions, color)
```

### 3. Entity Management (`entity/`)

#### `EntityManager` Responsibilities
- Periodic entity list updates from game memory
- Entity filtering (enemies only, alive players only, etc.)
- Render call delegation to visualization system

#### Configuration
```cpp
struct Config {
    bool render_aabb;
    bool render_health_bar;
    bool render_team_color;
    bool render_enemies_only;
    float update_interval_ms;  // Throttle memory reads
};
```

### 4. AABB Calculation

AABB (Axis-Aligned Bounding Box) is calculated from bone positions:

```cpp
AABB calculate_aabb(uintptr_t entity_ptr) {
    // Read bone transform matrices
    Vector3 bones[] = {
        head_bone,
        neck_bone,
        chest_bone,
        pelvis_bone,
        left_foot,
        right_foot
    };
    
    // Find min/max coordinates
    Vector3 min = first_bone;
    Vector3 max = first_bone;
    
    for (auto& bone : bones) {
        min = {min(min.x, bone.x), ...}
        max = {max(max.x, bone.x), ...}
    }
    
    // Add padding and return
    return AABB(min - padding, max + padding);
}
```

**Benefits:**
- More accurate than static box
- Follows player animation
- Supports mesh deformation

**Rendering (8-corner cube):**
```
7 -------- 6
|         |
4 -------- 5
|         |
3 -------- 2
|         |
0 -------- 1
```
- 4 edges per face (bottom, top, front, back)
- 4 vertical connecting edges
- **Total: 12 lines drawn**

## Data Flow Diagram

```
┌─────────────────┐
│  Game Process   │
│   (CS2.exe)     │
│                 │
│ ┌─────────────┐ │
│ │ Entity List │ │
│ │ Position    │ │
│ │ Health      │ │
│ │ Bone Matrix │ │
│ └─────────────┘ │
└────────┬────────┘
         │
         ↓ Memory Read
    ┌─────────────────┐
    │ MemoryReader    │
    │ (Platform)      │
    └────────┬────────┘
             │
             ↓
    ┌─────────────────┐
    │ GameMemory      │
    │ (Parse/Cache)   │
    └────────┬────────┘
             │
             ↓
    ┌─────────────────┐
    │ EntityManager   │
    │ (Filter/Update) │
    └────────┬────────┘
             │
             ↓
    ┌─────────────────┐
    │ Visualizer      │
    │ (Transform)     │
    └────────┬────────┘
             │
             ↓
    ┌─────────────────┐
    │ Renderer        │
    │ (Draw/Present)  │
    └─────────────────┘
```

## Key Technical Concepts

### 1. Memory Offsets

Game offsets can change with patches. To find correct offsets:

1. **Reverse Engineering**
   - Use IDA Pro, Ghidra to analyze game binary
   - Find entity pool allocation patterns
   - Trace to structure definitions

2. **Pattern Scanning**
   ```cpp
   // Example: Signature scanning for dynamic offset
   uintptr_t entity_list = pattern_scan("49 8D 0C D8 ?? ?? ?? ??");
   ```

3. **Offset Validation**
   - Verify read values make sense (health 0-100, positions in level bounds)
   - Use dummy values initially, refine iteratively

### 2. Bone Matrices

Players have 25+ bones for skeletal animation:
- Head (6), Neck (5), Chest (10), Pelvis (0)
- Limbs (1-4: left arm/hand, 7-8: right arm/hand)
- Legs (11-12: right/left foot, 13-14: toes)

Each bone stores a **4×4 transformation matrix**:
```
[Rotation   | Position ]
[-----------|----------]
[0  0  0 | 12 13 14]    ← Position (X, Y, Z)
[1  1  1 | -  -  - ]
[2  2  2 | -  -  - ]
[3  3  3 | -  -  - ]
```

### 3. World-to-Screen Projection

**Perspective division** converts 3D to 2D:

```cpp
// Simplified version
screen_x = (width / 2) + (world_x / world_z) * (width / (2 * tan(fov/2)))
screen_y = (height / 2) - (world_y / world_z) * (height / (2 * tan(fov/2)))
```

Full implementation requires:
- View matrix (camera position + rotation)
- Projection matrix (FOV, aspect ratio)
- Homogeneous division (divide by w)

### 4. Team Colors

```
Team 2 (Terrorist):          Orange (200, 100, 0)
Team 3 (Counter-Terrorist):  Light Blue (100, 150, 255)
Local Player:                Bright Green (0, 255, 0)
Unknown/Spectator:           White (255, 255, 255)
```

## Usage Example

```cpp
// 1. Initialize overlay
auto overlay = std::make_shared<Overlay>();
Overlay::Config config{};
config.entity_config.render_aabb = true;
overlay->initialize(config);

// 2. Run in background thread
std::thread t([overlay]() { overlay->run(); });

// 3. Configure rendering
overlay->set_render_callback([](auto renderer) {
    // Draw custom UI elements
    renderer->draw_text({10, 10}, "FPS: 144", {255, 255, 255});
});

// 4. Monitor entities
auto& entities = overlay->get_entity_manager()->get_entities();
for (const auto& e : entities) {
    std::cout << "Player: Health=" << e.health << std::endl;
}

// 5. Shutdown
overlay->stop();
t.join();
```

## Building

### Requirements
- C++20 compiler (MSVC, GCC, Clang)
- CMake 3.16+
- Platform-specific headers (Windows SDK / Linux kernel headers)

### Build Steps
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
./cs2overlay
```

## Security & Legal Considerations

**⚠️ Warning:** External overlays may violate:
- Game Terms of Service
- Anti-cheat policies (VAC, EAC, Ricochet)
- Computer fraud/abuse laws

**This is for educational purposes only.** Use only for:
- Learning game development concepts
- Practicing graphics programming
- Personal offline experimentation

## Future Enhancements

1. **Rendering Backends**
   - DirectX 11/12 implementation
   - OpenGL renderer
   - Vulkan high-performance renderer

2. **Advanced Features**
   - Skeleton visualization with bone lines
   - Prediction lines for moving players
   - Distance/velocity estimation
   - Weapon identification

3. **Performance Optimization**
   - Multi-threaded memory reading
   - GPU-accelerated projection
   - Temporal coherence caching
   - Culling by screen space

4. **Robustness**
   - Handle version updates
   - Signature-based offset scanning
   - Graceful degradation on read failures
   - Hot-reload capability

## References

- **Game Memory Concepts**: [Cheat Engine Wiki](http://wiki.cheatengine.org/)
- **Graphics Projection**: Real-time Rendering by Möller, Haines, Hoffman
- **Skeletal Animation**: [Game Developer Magazine Articles](https://www.gamedeveloper.com/)
- **Reverse Engineering**: Introduction to Reverse Engineering (Ghidra, IDA Pro)

---

**Project Status:** Conceptual Framework - Educational Demonstration

**Last Updated:** May 2026

**Author Note:** This framework prioritizes clarity and education over performance or feature completeness. Real production overlays would require significant optimization, error handling, and platform-specific considerations.
