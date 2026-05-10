# Development Roadmap & Architecture Diagrams

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                       CS2 Overlay Framework                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    MAIN APPLICATION                       │   │
│  │               (Overlay::run() main loop)                  │   │
│  └────┬─────────────────────────┬──────────────────────┬────┘   │
│       │                         │                      │          │
│       ↓                         ↓                      ↓          │
│  ┌─────────────┐        ┌──────────────┐      ┌─────────────┐   │
│  │   Memory    │        │   Entity     │      │   Render    │   │
│  │   Reader    │        │   Manager    │      │   System    │   │
│  └─────┬───────┘        └──────┬───────┘      └─────┬───────┘   │
│        │                       │                    │            │
│        ↓                       ↓                    ↓            │
│  ┌──────────────┐     ┌──────────────┐    ┌───────────────┐     │
│  │Game Memory   │     │Visualizers   │    │   Renderer    │     │
│  │Interface     │     ├──────────────┤    │  Primitives   │     │
│  │              │     │ - Health Bar │    │               │     │
│  │- Positions   │     │ - AABB Box   │    │ - draw_line   │     │
│  │- Health      │     │ - Skeleton   │    │ - draw_rect   │     │
│  │- Bones       │     │ - Team Color │    │ - draw_circle │     │
│  │- Team        │     └──────────────┘    │ - draw_text   │     │
│  └──────────────┘                         └───────────────┘     │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              Platform Abstraction Layer                  │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  Windows          │  Linux           │  Mock (Testing)   │   │
│  │  - ReadProcess    │  - /proc/[pid]   │  - Simulated      │   │
│  │  - Memory API     │  - /mem file     │  - Fake Data      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow Diagram

```
┌──────────────┐
│  CS2 Process │
│              │
│ ┌──────────┐ │
│ │Entities  │ │
│ │Position  │ │
│ │Health    │ │
│ │Bones     │ │
│ └──────────┘ │
└──────┬───────┘
       │ Physical Memory
       │ (ReadProcessMemory / /proc/mem)
       ↓
┌──────────────────────────────────────┐
│      MemoryReader::read_memory()      │
│  (Platform-specific implementation)   │
└──────────────┬───────────────────────┘
               │
               ↓
┌──────────────────────────────────────┐
│   GameMemory::read_players()          │
│                                       │
│  ├─ get_entity_from_list()           │
│  ├─ read_player_position()           │
│  ├─ read_player_health()             │
│  ├─ read_player_team()               │
│  ├─ read_bone_matrices()             │
│  └─ calculate_aabb()                 │
└──────────────┬───────────────────────┘
               │ PlayerEntity structs
               ↓
┌──────────────────────────────────────┐
│   EntityManager::update()             │
│                                       │
│  ├─ Throttle updates (16ms)          │
│  ├─ Filter entities (alive, visible) │
│  └─ Cache for rendering              │
└──────────────┬───────────────────────┘
               │ Filtered entities
               ↓
┌──────────────────────────────────────┐
│   EntityManager::render()             │
│                                       │
│  ├─ PlayerVisualizer::draw_player()  │
│  │   ├─ AABBVisualizer::draw_3d()   │
│  │   ├─ HealthBarVisualizer::draw()  │
│  │   └─ Team color indicator         │
│  └─ Collect render commands          │
└──────────────┬───────────────────────┘
               │ Screen-space coordinates
               ↓
┌──────────────────────────────────────┐
│   OverlayRenderer::render_frame()    │
│                                       │
│  ├─ begin_frame()                    │
│  ├─ draw_line() [12x for AABB]       │
│  ├─ draw_filled_rect() [healthbar]   │
│  ├─ draw_box_2d() [border]           │
│  ├─ draw_text() [info]               │
│  └─ end_frame()                      │
└──────────────┬───────────────────────┘
               │ Render commands
               ↓
      ┌────────────────────┐
      │  Graphics API      │
      │  (DirectX/OpenGL)  │
      └────────┬───────────┘
               │
               ↓
         ┌──────────────┐
         │ Screen Buffer│
         └──────────────┘
```

## 3D-to-2D Projection Pipeline

```
World Space (3D)
    ├─ Player Position: (X, Y, Z)
    ├─ AABB Corners: 8 vertices
    └─ Bone Positions: 25+ vertices
         │
         ↓ Apply View Matrix
         │ (Camera transform)
         ↓
View Space (3D)
    └─ Camera-relative coordinates
         │
         ↓ Apply Projection Matrix
         │ (Perspective division)
         ↓
Clip Space (3D w/ homogeneous coords)
    └─ Normalized device coordinates
         │
         ↓ Perspective Division (/ w)
         │ (Project to plane)
         ↓
NDC Space (-1 to 1)
    └─ Device-independent coordinates
         │
         ↓ Viewport Transform
         │ (Scale to screen resolution)
         ↓
Screen Space (2D)
    ├─ X: 0 to width
    └─ Y: 0 to height
```

## AABB Rendering (8 Corners & 12 Edges)

```
       7 -------- 6
      /|         /|
     / |        / |
    4 -------- 5  |
    |  3 -----|-- 2
    | /       | /
    |/        |/
    0 -------- 1

Corners:
  0: (min_x, min_y, min_z)
  1: (max_x, min_y, min_z)
  2: (max_x, max_y, min_z)
  3: (min_x, max_y, min_z)
  4: (min_x, min_y, max_z)
  5: (max_x, min_y, max_z)
  6: (max_x, max_y, max_z)
  7: (min_x, max_y, max_z)

Edges (12 total):
  Bottom face:  0-1, 1-2, 2-3, 3-0
  Top face:    4-5, 5-6, 6-7, 7-4
  Vertical:    0-4, 1-5, 2-6, 3-7
```

## Skeletal Bone Structure (Simplified)

```
                Head (6)
                  ↓
                Neck (5)
                  ↓
              Chest (10)
                  ↓
        ┌─────────┼─────────┐
        ↓         ↓         ↓
    L.Arm(1)  Pelvis(0)  R.Arm(4)
        ↓                   ↓
    L.Hand(2)           R.Hand(7)
        
    Pelvis (0)
        ↓
    ┌───┴───┐
    ↓       ↓
 L.Leg(13) R.Leg(12)
    ↓       ↓
 L.Foot(14) R.Foot(11)
```

## Health Bar Color Gradient

```
Health %   Color         RGB
─────────────────────────────
100%    → Green      →  (0, 255, 0)
 75%    → Yellow-G   → (64, 255, 0)
 50%    → Yellow     → (255, 255, 0)
 25%    → Orange     → (255, 165, 0)
  0%    → Red        → (255, 0, 0)

Formula:
if (health > 50%):
    R = 255 * (health - 0.5) * 2
    G = 255
    B = 0
else:
    R = 255
    G = 255 * health * 2
    B = 0
```

## Threading Model

```
┌─────────────────────────────────────────────┐
│           Main Application Thread           │
├─────────────────────────────────────────────┤
│                                              │
│  loop {                                      │
│    memory_lock.lock()                        │
│    entities = cached_entities  ← Shared     │
│    memory_lock.unlock()                      │
│                                              │
│    render_frame(entities)                    │
│    sleep(16ms)                               │
│  }                                           │
└─────────────────────────────────────────────┘
              ↑         ↑
              │ mutex   │ shared data
              ↓         ↓
┌─────────────────────────────────────────────┐
│    Background Memory Reader Thread           │
├─────────────────────────────────────────────┤
│                                              │
│  loop {                                      │
│    players = game_memory.read_players()      │
│    memory_lock.lock()                        │
│    cached_entities = players      ← Update   │
│    memory_lock.unlock()                      │
│    sleep(33ms)                               │
│  }                                           │
│                                              │
└─────────────────────────────────────────────┘
```

## Component Interaction Sequence

```
Time →

Main Loop                  Entity Manager             Visualizer
  │                              │                         │
  ├─ Update Entities() ──────────>│                         │
  │                              │                         │
  │                              ├─ Read Memory            │
  │                              │                         │
  │                              ├─ Parse Entities         │
  │                              │                         │
  │                              ├─ Filter/Cache           │
  │                              │                         │
  ├─ Render() ────────────────────────────────────>│        │
  │                                                │        │
  │                                                ├─ Calculate 3D→2D
  │                                                │        │
  │                                                ├─ Generate Draw Calls
  │                                                │        │
  │<──────────────────────────────────── Return   │        │
  │                                                │        │
  ├─ Submit to GPU ────────────────────────>Renderer│        │
  │                                          │             │
  │                                          ├─ Execute    │
  │                                          │             │
  │                                          ├─ Present    │
  │                                          │             │
  ├─ Sleep(16ms)                                    │        │
  │                                                 │        │
  (repeat)                                          │        │
```

---

See [FRAMEWORK_DESIGN.md](FRAMEWORK_DESIGN.md) for detailed documentation.
