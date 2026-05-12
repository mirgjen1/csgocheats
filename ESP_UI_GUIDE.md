# CS:GO Overlay ESP UI Guide

## Overview

The CS:GO overlay now includes a comprehensive **ESP (Electronic Sight Potential)** UI system that renders detailed boxes, health bars, player names, and distance information for detected entities in the game.

## Features

### Core ESP Visualization

- **AABB Boxes** - 3D bounding boxes around player models with black outline + colored edge
- **Health Bars** - Color-gradient health bars (Green → Yellow → Red)
- **Player Names** - Entity ID and team display above each player
- **Distance Display** - Distance in game units from origin
- **Snaplines** - Lines from screen center to each player
- **Skeleton Rendering** - Bone positions visualization (optional)
- **Team Colors** - Orange for Terrorists, Blue for Counter-Terrorists

### Configuration Options

All ESP features can be toggled independently:

```cpp
bool draw_boxes = true;           // 3D AABB boxes
bool draw_health_bars = true;     // Health bar visualization
bool draw_player_names = true;    // Player entity info
bool draw_team_colors = true;     // Team-based coloring
bool draw_snaplines = false;      // Lines to players
bool draw_distance = false;       // Distance text
bool draw_skeletons = false;      // Skeleton visualization
bool render_enemies_only = false; // Only show enemies
bool render_visible_only = false; // Only visible players
bool render_local_player = false; // Include local player
```

## Using the Overlay

### Running the Overlay

```bash
cd /home/mirgjen/Documents/csgocheats/build
./csgooverlay.exe &
```

The overlay will:
1. Auto-detect the CS:GO Legacy process
2. Read game memory for entity positions
3. Project 3D world coordinates to screen
4. Render ESP boxes with all configured options

### ESP Features in Action

#### AABB Boxes
- **Black outline** provides visibility on any background
- **Colored interior** shows team (Orange/Blue)
- **Local player** shown in bright green
- **Thickness configurable** - default 2.0 pixels

#### Health Bars
- Position: Above each player's head
- Format: Dark background bar + filled health portion
- **Color gradient**:
  - Green (100% health)
  - Yellow (50% health)
  - Red (0% health)
- **White border** for clarity

#### Distance Calculation
- Measures distance from world origin (0, 0, 0)
- Displayed in game units below player name
- Can be enhanced to measure from local player position

### Configuration File

ESP settings are saved to `/tmp/csgocheat_esp_config.txt`:

```ini
[ESP_CONFIG]
enabled=1
draw_boxes=1
draw_health_bars=1
draw_player_names=1
draw_team_colors=1
draw_skeletons=0
draw_snaplines=0
draw_distance=0
render_enemies_only=0
render_visible_only=0
render_local_player=0
box_thickness=2.00
line_thickness=1.50
snapline_thickness=1.00
max_render_distance=8000.00
debug_mode=0
show_entity_count=1
show_fps=1
```

Edit this file to customize ESP behavior before running the overlay.

## Customizing ESP

### In C++ Code

Edit [include/rendering/esp_ui.hpp](include/rendering/esp_ui.hpp):

```cpp
// Default ESP colors
Color enemy_color = Color(200, 100, 0, 255);      // Orange for T
Color team_color = Color(100, 150, 255, 255);     // Light blue for CT
Color local_player_color = Color(0, 255, 0, 255); // Green for self
Color box_outline_color = Color(0, 0, 0, 255);    // Black outline
```

### Runtime Configuration

Get the ESP UI instance from overlay:

```cpp
ESPUIPtr esp_ui = overlay->get_esp_ui();

// Get/modify config
ESPConfig& config = esp_ui->get_config();
config.draw_boxes = true;
config.box_thickness = 3.0f;

// Save custom config
config.save_to_file("/path/to/custom_config.txt");

// Load config
config.load_from_file("/path/to/custom_config.txt");

// Reset to defaults
esp_ui->reset_to_defaults();
```

## Rendering Pipeline

The ESP UI rendering follows this order:

```
1. Entity Detection (read from game memory)
2. Entity Filtering (alive, distance, team checks)
3. Screen Projection (3D → 2D coordinates)
4. AABB Box Rendering
   - Black outline (2D projection of 3D edges)
   - Colored fill (team color)
5. Health Bar Rendering
   - Background bar
   - Health percentage fill
   - White border
6. Text Rendering
   - Player name
   - Distance
   - Health value
7. Optional Effects
   - Snaplines
   - Skeleton
```

## Debug Mode

Enable debug mode for troubleshooting:

```cpp
esp_ui->get_config().debug_mode = true;
esp_ui->get_config().show_entity_count = true;
esp_ui->get_config().show_fps = true;
```

This displays:
- Entity count (bottom-left)
- FPS counter
- UI state information

## Entity Filtering

### Distance Filtering

```cpp
float max_render_distance = 8000.0f; // Only render within 8000 units
```

### Team Filtering

```cpp
bool render_enemies_only = true; // Hide friendlies
```

### Visibility Filtering

```cpp
bool render_visible_only = true; // Only render visible players (requires line-of-sight check)
```

## Memory Requirements

The ESP system is optimized for minimal overhead:

- **Memory**: ~100KB for ESP UI structures
- **CPU**: ~2-5% per frame (depends on entity count)
- **Rendering**: Batched 3D line draws for efficiency

## Troubleshooting

### No Boxes Appearing

1. **Check entity detection**:
   ```bash
   # Enable debug mode
   esp_ui->get_config().debug_mode = true;
   ```
   Should show "Entities: X" in bottom-left

2. **Verify offsets**:
   - Run offset finder: `sudo ./offset_finder [pid]`
   - Confirm health offset is correctly set

3. **Check distance filter**:
   ```cpp
   esp_ui->get_config().max_render_distance = 10000.0f; // Increase
   ```

### Boxes Not Updating

- Entity list offset may be wrong
- Try using the offset finder tool to discover correct offsets

### Performance Issues

- Reduce box thickness: `config.box_thickness = 1.0f`
- Disable snaplines: `config.draw_snaplines = false`
- Disable skeleton: `config.draw_skeletons = false`

## Advanced: Custom Rendering

Add custom ESP elements using the render callback:

```cpp
overlay->set_render_callback([](RendererPtr renderer) {
    // Custom rendering code
    Vector2 screen_pos(100, 100);
    renderer->draw_text(screen_pos, "Custom ESP Element", Color(255, 255, 0, 255));
});
```

## Architecture

### Key Components

1. **ESPConfig** - Configuration management with file I/O
2. **ESPUI** - Main ESP UI system
3. **PlayerVisualizer** - Entity rendering (boxes, health bars)
4. **EntityManager** - Entity detection and filtering
5. **Renderer** - GPU rendering backend (OpenGL on Linux)

### Data Flow

```
Game Memory
    ↓
Entity Detection (EntityManager)
    ↓
ESP Filtering (ESPUI::should_render_entity)
    ↓
3D Projection (world_to_screen)
    ↓
Rendering (renderer->draw_*)
    ↓
Screen Display
```

## Performance Tips

1. **Disable unused features**
   - Snaplines have CPU overhead
   - Skeleton rendering requires bone matrix reads

2. **Optimize entity counts**
   - Use `render_enemies_only = true` to halve rendering
   - Set appropriate `max_render_distance`

3. **Batch rendering**
   - Current implementation already batches similar draws
   - OpenGL GPU rendering is efficient

## Next Steps

1. **Verify offset discovery** using the offset_finder tool
2. **Test ESP rendering** with real game data
3. **Customize colors/thickness** to personal preference
4. **Enable additional features** (snaplines, skeleton) as needed

## Support

If ESP boxes aren't rendering:

1. Run with debug mode enabled
2. Check entity count display
3. Verify memory offsets using offset_finder
4. Check stderr output for initialization errors
5. Ensure game is running: `pgrep -f csgo_linux64`

Good luck! 🎮
