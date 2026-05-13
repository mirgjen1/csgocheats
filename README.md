# CS2 External Overlay Framework

A conceptual C++ framework for building external game overlays with real-time entity rendering, AABB visualization, and dynamic health bar updates. This project demonstrates core graphics programming and game memory access concepts.

## Features

**Core Capabilities**
- Real-time game memory reading (Windows/Linux)
- Dynamic player entity tracking
- 3D-to-2D world-to-screen projection
- AABB (Axis-Aligned Bounding Box) rendering
- Dynamic health bar visualization with color gradients
- Team-based entity coloring (T/CT)
- Skeletal bone matrix reading
- Multi-threaded rendering loop

## Project Structure

```
CS2Overlay/
├── include/
│   ├── game/              # Game data structures and memory offsets
│   ├── memory/            # Memory reading interfaces
│   ├── rendering/         # Rendering abstractions
│   ├── entity/            # Entity management
│   └── overlay/           # Main overlay application
├── src/
│   ├── memory/            # Memory reader implementations
│   ├── rendering/         # Renderer implementations
│   ├── entity/            # Entity manager implementation
│   ├── overlay/           # Overlay implementation
│   └── main.cpp           # Example usage
├── FRAMEWORK_DESIGN.md    # Comprehensive design documentation
└── IMPLEMENTATION_GUIDE.md # Practical implementation guide
```

## Architecture Overview

```
Game Memory → MemoryReader → GameMemory → EntityManager → Visualizer → Renderer → Screen
```

### Component Responsibilities

| Component | Purpose |
|-----------|---------|
| **MemoryReader** | Raw memory access (platform-specific) |
| **GameMemory** | Game-specific data parsing and offset resolution |
| **EntityManager** | Entity lifecycle, filtering, and update scheduling |
| **Visualizer** | Transform game data to visual representations |
| **Renderer** | Render primitives to screen (graphics API) |
| **Overlay** | Orchestrate all components in main loop |

## Key Concepts

### 1. AABB Rendering
Renders axis-aligned bounding boxes around player entities by:
1. Reading bone transform matrices from game memory
2. Calculating min/max coordinates from bone positions
3. Projecting 8 corners to screen space
4. Drawing 12 edges connecting corners

### 2. Health Bar Visualization
Dynamic health bars with:
- **Color gradient**: Green (100%) → Yellow (50%) → Red (0%)
- **Background frame**: Dark with border
- **Position**: Above player head in screen space
- **Update rate**: Tied to memory read frequency

### 3. Memory Management
- **Abstraction-based**: Multiple memory readers for different platforms
- **Type-safe**: Templated reading with proper structure alignment
- **Offset-based**: All game data access via configurable offsets
- **Fallback support**: Mock reader for testing

### 4. Threading Model
- **Main thread**: Orchestrates rendering and entity updates
- **Optional background threads**: For memory reading
- **Synchronization**: Mutex protection for shared entity data

## Building

### Prerequisites
- C++20 compiler (MSVC, GCC, or Clang)
- CMake 3.16 or higher
- Platform-specific development tools:
  - **Windows**: Windows SDK
  - **Linux**: kernel headers (`linux-headers` package)

### Build Instructions

```bash
# Clone repository
cd ~/Documents/csgocheats

# Create build directory
mkdir build && cd build

# Configure CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build project
cmake --build . --config Release

# Run
./csgooverlay # (sudo when in linux to properly read memory and the process csgo_linux64)
```

## Usage

### Basic Example

```cpp
#include "overlay/overlay.hpp"

int main() {
    // Create and configure overlay
    auto overlay = std::make_shared<Overlay>();
    
    Overlay::Config config;
    config.window_width = 1920;
    config.window_height = 1080;
    config.entity_config.render_aabb = true;
    config.entity_config.render_health_bar = true;
    
    // Initialize
    if (!overlay->initialize(config)) {
        return 1;
    }
    
    // Run in background thread
    std::thread t([overlay]() { overlay->run(); });
    
    // Add custom rendering
    overlay->set_render_callback([](auto renderer) {
        renderer->draw_text({10, 10}, "CS2 Overlay", {255, 255, 255});
    });
    
    // Monitor entities
    auto entities = overlay->get_entity_manager();
    while (overlay->is_running()) {
        std::cout << "Entities: " << entities->entity_count() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    overlay->stop();
    t.join();
    return 0;
}
```

### Configuration

```cpp
EntityManager::Config config;
config.render_aabb = true;           // Draw bounding boxes
config.render_health_bar = true;     // Draw health bars
config.render_team_color = true;     // Color by team
config.render_enemies_only = false;  // Include all players
config.update_interval_ms = 16.0f;   // Update frequency (~60 FPS)
```

## Memory Offsets

The framework uses configurable offsets defined in [game_structures.hpp](include/game/game_structures.hpp):

```cpp
ENTITY_LIST         // Base address of entity array
LOCAL_PLAYER        // Local player entity pointer
PLAYER_POSITION     // Vector3 position offset
PLAYER_HEALTH       // Health value offset
PLAYER_TEAM         // Team information offset
PLAYER_BONE_MATRIX  // Skeletal transform matrices offset
```

**Important:** These are example offsets that may vary by game version. See [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) for offset discovery procedures.

## Documentation

- **[FRAMEWORK_DESIGN.md](FRAMEWORK_DESIGN.md)** - Comprehensive architecture and design patterns
- **[IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)** - Practical implementation details and troubleshooting

## Rendering Backends

The framework uses an abstract `Renderer` interface supporting multiple backends:

- **DirectX 11/12** (Windows)
- **OpenGL** (Cross-platform)
- **Vulkan** (High-performance)
- **Mock Renderer** (Testing/Demonstration)

Currently implemented: **Mock/Placeholder Renderer**

To implement a real renderer, inherit from `Renderer` and implement:
```cpp
class MyRenderer : public Renderer {
    bool initialize(uint32_t width, uint32_t height) override;
    void draw_line(const Vector2& from, const Vector2& to, 
                   const Color& color, float thickness) override;
    // ... other primitives
};
```

## Performance Considerations

- **Memory reads**: ~1-2ms per 64 players (configurable interval)
- **Rendering**: Depends on entity count and renderer implementation
- **Target**: 60+ FPS with 50+ players visible
- **Optimization**: Screen culling, distance-based LOD, threaded I/O

## Educational Value

This project teaches:

1. **Game Development**
   - Entity management systems
   - Graphics projection and rendering
   - Memory-driven gameplay logic

2. **Graphics Programming**
   - Coordinate system transformations
   - Projection mathematics
   - Screen-space rendering optimization

3. **Systems Programming**
   - Process memory access
   - Platform-specific APIs
   - Multi-threaded synchronization

4. **Software Architecture**
   - Abstract interfaces and polymorphism
   - Dependency injection patterns
   - Component-based design

## Limitations & Disclaimers

**Educational Purposes Only**

This framework is for learning graphics and systems programming concepts. External overlays may:

- Violate game Terms of Service
- Trigger anti-cheat detection systems
- Break game balance or fair play
- Violate laws in some jurisdictions

**Use at your own risk and only for legitimate purposes.**

## References

- Real-time Rendering (Möller, Haines, Hoffman)
- Game Engine Architecture (Gregory)
- Cheat Engine reverse engineering techniques
- DirectX 11 documentation
- OpenGL specification

## License

Educational/Research purposes. See LICENSE file for details.

---

**Questions or Contributions?**
This is a framework for learning. Feel free to extend it with:
- Additional rendering backends
- More sophisticated AABB calculations
- Performance optimizations
- Advanced visualization features

**Last Updated:** May 2026
