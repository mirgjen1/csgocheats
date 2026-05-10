# Quick Start Guide

## 5-Minute Setup

### 1. Prerequisites
- Windows 10+ or Linux
- Visual Studio 2022 / GCC 11+ / Clang 14+
- CMake 3.16+

### 2. Clone and Build

```bash
# Navigate to project
cd ~/Documents/csgocheats

# Create build directory
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release

# Run
./cs2overlay
```

## Understanding the Code Structure

### Entry Point
**[src/main.cpp](src/main.cpp)** - Demonstrates basic overlay usage:
```cpp
1. Create Overlay instance
2. Configure entity rendering options
3. Initialize overlay system
4. Set custom render callbacks
5. Run in background thread
6. Monitor entity updates
7. Shutdown gracefully
```

### Memory Access
**[include/memory/game_structures.hpp](include/game/game_structures.hpp)** - Defines:
- Memory offsets for CS2 data structures
- Player entity representation
- Transform matrices and geometry types

**[src/memory/game_memory.cpp](src/memory/game_memory.cpp)** - Implements:
- `read_players()` - Get all living player entities
- `read_player_health()` - Extract health value
- `calculate_aabb()` - Generate bounding box from bones
- `read_bone_matrix()` - Access skeletal transforms

### Rendering System
**[include/rendering/renderer.hpp](include/rendering/renderer.hpp)** - Defines interface:
- Abstract drawing primitives
- 3D-to-2D projection
- Color and geometry types

**[src/rendering/renderer.cpp](src/rendering/renderer.cpp)** - Implements:
- World-to-screen transformation
- AABB corner projection
- Rendering pipeline

### Visualization
**[include/rendering/visualizer.hpp](include/rendering/visualizer.hpp)** - Provides:
- `PlayerVisualizer::draw_player()` - Complete player rendering
- `HealthBarVisualizer::draw_health_bar()` - Health display
- `AABBVisualizer::draw_3d_aabb()` - Bounding box drawing

## Common Customizations

### Change Update Frequency
```cpp
// Make it update faster (more CPU usage)
config.entity_config.update_interval_ms = 8.0f;  // 125 FPS

// Or slower (less CPU usage, less responsive)
config.entity_config.update_interval_ms = 33.0f; // 30 FPS
```

### Disable Features
```cpp
config.entity_config.render_aabb = false;         // No boxes
config.entity_config.render_health_bar = false;   // No health bars
config.entity_config.render_enemies_only = true;  // Only enemies
```

### Add Custom Rendering
```cpp
overlay->set_render_callback([](RendererPtr renderer) {
    // Draw FPS counter
    static int fps = 0;
    fps++;
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "FPS: %d", fps);
    renderer->draw_text({10, 10}, buffer, {0, 255, 0, 255});
    
    // Draw crosshair
    auto w = renderer->get_width();
    auto h = renderer->get_height();
    Vector2 center(w / 2.0f, h / 2.0f);
    renderer->draw_line(
        {center.x - 5, center.y},
        {center.x + 5, center.y},
        {255, 0, 0, 255},
        2.0f
    );
});
```

## Debugging

### Enable Debug Output
```cpp
// Add diagnostic rendering
void render_debug() {
    if (overlay->get_entity_manager()) {
        auto count = overlay->get_entity_manager()->entity_count();
        std::cout << "Entities: " << count << std::endl;
    }
}
```

### Check Memory Access
```cpp
// Verify memory reader is working
auto reader = std::make_shared<MockMemoryReader>();
uint32_t test_value = reader->read<uint32_t>(0x1000);
std::cout << "Read test value: " << test_value << std::endl;
```

### Profile Performance
```cpp
auto start = std::chrono::high_resolution_clock::now();
// ... render frame ...
auto end = std::chrono::high_resolution_clock::now();
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Frame time: " << ms.count() << "ms" << std::endl;
```

## Next Steps

1. **Find Real Offsets**
   - Use Cheat Engine to locate correct memory offsets
   - Update [game_structures.hpp](include/game/game_structures.hpp)
   - See [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) for details

2. **Implement Renderer**
   - Choose DirectX 11, OpenGL, or Vulkan
   - Implement drawing primitives
   - Test with mock data first

3. **Add Features**
   - Distance calculation
   - Weapon identification
   - Skill indicators
   - Prediction lines

4. **Optimize**
   - Profile hot paths
   - Add threading for memory reads
   - Implement viewport culling
   - Batch render commands

## Useful Resources

- [FRAMEWORK_DESIGN.md](FRAMEWORK_DESIGN.md) - Complete architecture
- [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) - Practical implementation details
- [ARCHITECTURE.md](ARCHITECTURE.md) - Visual diagrams and data flow

## Troubleshooting

**Q: Nothing renders**
- A: Check if memory offsets are correct (use Cheat Engine)
- A: Verify renderer backend is initialized
- A: Ensure renderer->is_ready() returns true

**Q: Bounding boxes are wrong positions**
- A: PLAYER_POSITION offset is likely incorrect
- A: Use Cheat Engine to validate position values
- A: Check Vector3 struct alignment (12 bytes)

**Q: High CPU usage**
- A: Increase update_interval_ms
- A: Reduce entity count limit
- A: Enable culling for off-screen entities

**Q: Compilation errors**
- A: Use C++20 compiler
- A: Install platform development headers
- A: Check CMakeLists.txt for your platform

---

**Happy coding!** 🎮

For more details, see [README.md](README.md)
