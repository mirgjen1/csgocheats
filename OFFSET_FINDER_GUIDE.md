# CS:GO Legacy Offset Finder Guide

## Overview

The offset finder is an automated tool that discovers game memory offsets by scanning for known values. Instead of using Cheat Engine (unavailable on Linux), this tool uses iterative value narrowing to pinpoint where game data is stored in memory.

## Prerequisites

- CS:GO Legacy running through Proton
- Root/sudo access (to read `/proc/[pid]/mem`)
- The overlay built (using `./offset_finder` requires root)

## Quick Start

### Step 1: Find the CS:GO Process ID

```bash
pgrep -f csgo_linux64
```

This will print a number like `12345`. This is the PID.

### Step 2: Run the Offset Finder

```bash
cd /home/mirgjen/Documents/csgocheats/build
sudo ./offset_finder 12345
```

Replace `12345` with your actual PID from Step 1.

### Step 3: First Scan - Current Health

When prompted:
```
Enter your CURRENT health value: 100
```

Enter whatever your current health is in the game (typically 100 at full health).

The tool will scan the entire game memory for this value. This takes ~30 seconds to a few minutes.

```
[OffsetFinder] Scanning for health value: 100
[OffsetFinder] This may take a minute...
  Scanning [heap] (0x564... - 0x567..., 50 MB)
  ...scanning...
[OffsetFinder] Found 523 possible locations

Address         Value   Description
=========================================
0x0000564a8a20  100     Possible health
0x0000564a9840  100     Possible health
... (more results)
```

### Step 4: Narrow Down Results

Now take damage in the game (get shot by a bot or an enemy). Once you have a new health value:

```
Current results: 523
Enter your CURRENT health value: 75
```

The tool narrows the results to only addresses where the value also changed to 75:

```
[OffsetFinder] Narrowing from 523 results...
[OffsetFinder] Narrowed to 12 results
```

### Step 5: Repeat Until You Have 1-5 Results

Take damage again and repeat:

```
Current results: 12
Enter your CURRENT health value: 50
```

```
[OffsetFinder] Narrowing from 12 results...
[OffsetFinder] Narrowed to 1 result
```

Once you have 1-5 results, the tool will display them:

```
=== FOUND OFFSETS ===
Address         Value   Description
=========================================
0x000056550a8c  50      Confirmed match
```

## Extracting the Offset

Once you have found the health offset, update the overlay:

### 1. Edit `include/game/game_structures.hpp`

Find the line with `ENTITY_LIST_OFFSET` and look for health offset patterns. Add your found offset:

```cpp
const uintptr_t HEALTH_OFFSET = 0x000056550a8c;  // Your found offset
```

### 2. Update the Entity Structure

In `include/entity/entity.hpp`, update the health reading to use the found offset:

```cpp
// Get health from the offset you found
uint32_t health = *(uint32_t*)((uintptr_t)entity_ptr + HEALTH_OFFSET);
```

### 3. Rebuild

```bash
cd /home/mirgjen/Documents/csgocheats
cmake -B build
cd build
make
```

### 4. Test the Overlay

```bash
cd /home/mirgjen/Documents/csgocheats/build
./cs2overlay &
```

The overlay should now:
- Detect entities (show > 0 entities in debug output)
- Display AABB boxes around players
- Show health bars with correct values and colors

## Troubleshooting

### "Failed to open /proc/[pid]/mem"

**Solution:** Run with `sudo`:
```bash
sudo ./offset_finder 12345
```

### "Found 0 possible locations"

Your health value isn't in game memory as a 32-bit unsigned integer. Possible causes:
- Health isn't stored as `uint32_t` (try `uint8_t` by modifying the code)
- You entered the wrong health value
- CS:GO Legacy stores health differently

Try re-running with correct health value or a different format.

### "Results won't narrow down"

- Health value isn't changing between scans (take more damage)
- The narrowing strategy requires different values each time
- Try multiple scans with different health values

## Advanced: Modifying the Tool

If you need to find other offsets (ammo, armor, etc.), the tool can be modified:

### In `src/memory/offset_finder.cpp`

The key function is `find_health_offsets()`. You can:
- Change the search value type (`uint32_t` → `uint8_t`, `uint16_t`, etc.)
- Change memory range filtering (add/remove certain regions)
- Add additional validation

Example for `uint8_t`:

```cpp
// In find_health_offsets - replace the search loop:
for (size_t i = 0; i < chunk.size() - sizeof(uint8_t); ++i) {
    uint8_t* value_ptr = (uint8_t*)&chunk[i];
    if (*value_ptr == health_value) {
        // Found a match
    }
}
```

## Next Steps

After finding and testing the health offset:

1. Test entity detection with the overlay running
2. If entities appear but health bars are wrong, verify the offset
3. Look for other game offsets (ammo, armor, team) using the same process
4. Update `include/game/game_structures.hpp` with all found offsets

## FAQ

**Q: Do I need root access?**
A: Yes, reading `/proc/[pid]/mem` requires root. Use `sudo`.

**Q: How long does scanning take?**
A: First scan takes ~30 seconds to 2 minutes depending on game memory size. Narrowing is instant.

**Q: Can I find other offsets?**
A: Yes! Modify the tool to scan for other values (ammo, armor, etc.).

**Q: Why does it find multiple matches?**
A: Game memory contains the same values in multiple places. Narrowing eliminates false positives.

**Q: What if health is stored as a float or byte?**
A: Modify `offset_finder.cpp` to search for `float` or `uint8_t` instead of `uint32_t`.

## Support

If the tool doesn't work:
1. Verify CS:GO Legacy is running: `pgrep -f csgo_linux64`
2. Verify PID is correct: `ps aux | grep csgo`
3. Run with sudo: `sudo ./offset_finder [pid]`
4. Check health value carefully - it must match exactly
5. Take enough damage between scans to narrow results

Good luck! 🎮
