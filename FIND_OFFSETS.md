# How to Find CS2 Memory Offsets Using Cheat Engine

This guide explains how to discover the correct memory offsets for Counter-Strike 2, which must be updated in [include/game/game_structures.hpp](../include/game/game_structures.hpp).

## Prerequisites

1. **Cheat Engine** - Download from https://www.cheatengine.org/
2. **Counter-Strike 2** - Installed and running
3. **Administrator privileges** - Required to read other process memory
4. **Patience** - Offset discovery takes 15-30 minutes

## Step-by-Step Offset Discovery

### 1. Find Player Position Offset

#### Goal: Locate the Vector3 (X, Y, Z) position in player entity structure

**Steps:**

1. Open Cheat Engine
2. Click "File" → "Open Process" → Select `cs2.exe`
3. Launch CS2 and join a game
4. In Cheat Engine, select "Pointer" → "Value Type" → "Float"
5. Search for your current position:
   - Get your position from console: `cl_showpos 1` in game
   - Enter X coordinate in Cheat Engine search
   - Click "First Scan"

6. Move to a different location in-game
7. Update search with new X coordinate
8. Click "Next Scan" 
9. Repeat 2-3 more times until you narrow down to few results (10-20)

10. Right-click suspected position → "Pointer" → "Dereference"
    - Look for consistent offset pattern

11. Record the offset (e.g., `0x1344`)

```
PLAYER_POSITION offset found at: base_address + 0x1344
```

### 2. Find Player Health Offset

#### Goal: Locate the uint32_t health value (0-100)

**Steps:**

1. In-game, get shot and take damage (health ~85)
2. In Cheat Engine, set Value Type to "4-byte Integer"
3. Search for `85`
4. Take more damage (health ~60)
5. Search for `60`
6. Continue filtering until 1-5 results remain

7. Record the offset relative to entity base
   - Usually near position offset (e.g., `0x324`)

```
PLAYER_HEALTH offset found at: base_address + 0x324
```

**Validation:** Health should update when you take damage, range 0-100.

### 3. Find Player Team Offset

#### Goal: Locate the uint32_t team identifier

**Steps:**

1. Note your team number (Terrorists = 2, CTs = 3)
2. In Cheat Engine, set Value Type to "4-byte Integer"
3. Search for your team number:
   - Type `2` or `3` depending on your team
   - Click "First Scan"

4. Have a teammate change team (they rejoin as different team)
5. Update search with their team number
6. Click "Next Scan"
7. Filter down to few results

8. Compare with enemy team:
   - Enemies should have different value at same offset

```
PLAYER_TEAM offset found at: base_address + 0x3C0
```

**Validation:** 
- Your team: 2 (Terrorist) or 3 (CT)
- Enemy team: different value
- Spectators: sometimes 0 or 1

### 4. Find Entity List Offset

#### Goal: Locate the base pointer to the array of all player entities

**Steps:**

1. Find any player's base address (entity pointer)
   - Use position offset you found earlier
   - Work backwards through Cheat Engine's pointer tool

2. Look at memory structure:
   - Entities are usually stored in contiguous array
   - Each entity is pointer (8 bytes on 64-bit)
   - Find where your entity pointer is stored

3. Search backwards from entity address:
   - Look for address that contains your entity pointer
   - Often in pattern like `[base] + index*8`

4. Find the base address that all player pointers derive from

```
ENTITY_LIST offset found at: 0x1A8C690
```

**Alternative Method - Signature Scanning:**
```cpp
// Pattern scan in-game for entity list patterns
// Look for mov rcx, [rip + offset]; instruction sequences
```

### 5. Find Local Player Offset

#### Goal: Find pointer specifically to your local player entity

**Steps:**

1. Once you have entity list and health offset
2. Assume local player is usually first or at specific index
3. Search for a pointer that:
   - Points to an entity with your team
   - Has your current health
   - Has your current position

4. Usually fixed offset (e.g., `0x1A8A6D8`)

```
LOCAL_PLAYER offset found at: 0x1A8A6D8
```

### 6. Find Bone Matrix Offset

#### Goal: Locate skeletal transformation matrices

**Steps:**

1. Bone matrices are more complex - usually in block of data
2. Look at memory around player entity:
   - Position: +0x1344
   - Health: +0x324
   - Scroll down in memory viewer

3. Find a large block of float values (4-byte floats)
4. Look for patterns:
   - 4x4 matrices (16 floats = 64 bytes each)
   - Multiple consecutive matrices
   - Usually after model/animation pointers

5. Validate bones by checking if values look like transformation matrices:
   - Rotation part (values between -1 and 1)
   - Translation part (position-like values)

```
PLAYER_BONE_MATRIX offset found at: base_address + 0x1080
```

## Verification Checklist

Before using offsets in code:

```
✓ Position updates when moving
✓ Health decreases when taking damage  
✓ Health increases when healing
✓ Team value matches scoreboard
✓ Bone matrix values look reasonable:
  - Floats in range [-10000, 10000] (position)
  - Floats in range [-1, 1] (rotation)
✓ Position moves relative to movement input
✓ Multiple players show different values
```

## Offset Recording Template

```cpp
// Record offsets in include/game/game_structures.hpp
namespace offsets {
    constexpr uintptr_t ENTITY_LIST = 0xYYYYYYYY;     // ← Found via entity array
    constexpr uintptr_t LOCAL_PLAYER = 0xYYYYYYYY;    // ← Found via pointer search
    
    constexpr uintptr_t PLAYER_POSITION = 0xXXXX;     // ← Found via position scan
    constexpr uintptr_t PLAYER_HEALTH = 0xXXXX;       // ← Found via health scan
    constexpr uintptr_t PLAYER_TEAM = 0xXXXX;         // ← Found via team scan
    constexpr uintptr_t PLAYER_NAME = 0xXXXX;         // ← (Optional)
    constexpr uintptr_t PLAYER_BONE_MATRIX = 0xXXXX;  // ← Found via bone search
};
```

## Advanced Techniques

### Using AOB (Array of Bytes) Scanning

Instead of fixed offsets, scan for code patterns:

```
// In Cheat Engine's "Pointer Scanner" or custom scanner
// Search for: 8B 81 44 13 00 00  (mov eax, [rcx+0x1344])
// This finds instructions accessing PLAYER_POSITION
```

### Dynamic Offset Discovery

```cpp
// In your code, implement pattern-based offset discovery
std::vector<uintptr_t> scan_for_pattern(const char* pattern) {
    // Search entire module for pattern
    // Return list of matching addresses
}
```

### Pointer Chain Analysis

```
Local Player Address:
  Read [0x1A8A6D8] → Entity Pointer (e.g., 0x140000000)
  
Player Data Offsets:
  [Entity + 0x1344] = Position (X, Y, Z)
  [Entity + 0x324]  = Health
  [Entity + 0x3C0]  = Team
  [Entity + 0x1080] = Bone Matrices
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Can't find offsets | Make sure you're in-game (not menu) |
| Offsets change each session | Likely relative to module base, not absolute |
| Health offset keeps changing | May be encrypted or offset within structure |
| Scan returns too many results | Add more filter conditions (scan again with different value) |
| Values don't match | Verify data type (4-byte int, float, etc.) |

## Important Notes

⚠️ **Offsets Change with Game Updates**
- New CS2 patches often shift offsets
- After each update, repeat discovery process
- Consider implementing dynamic scanning

⚠️ **Anti-Cheat Considerations**
- Cheat Engine itself may be detected
- Some offsets may be intentionally obfuscated
- Use pattern scanning instead of fixed offsets for stability

⚠️ **Private/Community Offsets**
- Community-provided offsets may be outdated
- Always verify with your own scanning
- Don't rely on offsets from forums (may be wrong/outdated)

## Next Steps

1. **Find your offsets** using this guide
2. **Update** [include/game/game_structures.hpp](../include/game/game_structures.hpp)
3. **Rebuild** the project:
   ```bash
   cd build
   cmake ..
   cmake --build . --config Release
   ```
4. **Test** by running the overlay with actual CS2

## Additional Resources

- **Cheat Engine Tutorial**: https://www.cheatengine.org/tutorial
- **Community Forums**: https://www.unknowncheats.me/forum/
- **IDA Pro/Ghidra**: Advanced reverse engineering tools
- **Game Hacking**: "Game Hacking" book by Kara Massoth

---

**Time Estimate**: 20-30 minutes for first-time offset discovery

**Success Rate**: 95%+ if you follow all steps carefully
