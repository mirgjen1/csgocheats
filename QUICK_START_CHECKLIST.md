# Quick Reference - Get Running in 1 Hour

## The 4 Steps to Running Overlay In-Game

### Step 1: Find Offsets (30 minutes)
```
1. Download Cheat Engine: cheatengine.org
2. Follow: FIND_OFFSETS.md (detailed guide)
3. Find these 6 values:
   - ENTITY_LIST
   - LOCAL_PLAYER
   - PLAYER_POSITION
   - PLAYER_HEALTH
   - PLAYER_TEAM
   - PLAYER_BONE_MATRIX
```

### Step 2: Update Code (2 minutes)
```
Edit: include/game/game_structures.hpp

namespace offsets {
    constexpr uintptr_t ENTITY_LIST = 0x1A8C690;      // ← YOUR VALUE
    constexpr uintptr_t LOCAL_PLAYER = 0x1A8A6D8;     // ← YOUR VALUE
    constexpr uintptr_t PLAYER_POSITION = 0x1344;     // ← YOUR VALUE
    constexpr uintptr_t PLAYER_HEALTH = 0x324;        // ← YOUR VALUE
    constexpr uintptr_t PLAYER_TEAM = 0x3C0;          // ← YOUR VALUE
    constexpr uintptr_t PLAYER_BONE_MATRIX = 0x1080;
};
```

### Step 3: Build (10 minutes)
```bash
cd ~/Documents/csgocheats
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Step 4: Run (5 minutes)
```bash
1. Launch Counter-Strike 2
2. Join a game
3. Right-click: build/Release/cs2overlay.exe
4. Select "Run as Administrator"
5. Look for green boxes around players!
```

---

## What Should Happen

✅ **Overlay window** appears (title: "CS2 Overlay")
✅ **Green boxes** around visible enemies
✅ **Health bars** above each player
✅ **Real-time updates** as players move

---

## Common Issues

| Issue | Solution |
|-------|----------|
| No overlay window | Run as Administrator |
| No players showing | Wrong offsets - rescan with Cheat Engine |
| Boxes in wrong places | PLAYER_POSITION offset wrong |
| Health bars empty | PLAYER_HEALTH offset wrong |
| Nothing changes | ENTITY_LIST offset wrong |

---

## Key Files

| File | Purpose |
|------|---------|
| [FIND_OFFSETS.md](FIND_OFFSETS.md) | How to find memory offsets |
| [RUNNING_IN_GAME.md](RUNNING_IN_GAME.md) | Detailed troubleshooting |
| [include/game/game_structures.hpp](include/game/game_structures.hpp) | Update offsets here |
| [src/main.cpp](src/main.cpp) | Customize behavior here |

---

## Testing Checklist

```
✓ Overlay window visible
✓ Transparent background
✓ Green boxes on players
✓ Health bars update when damage taken
✓ No crashes after 5 minutes
✓ CPU usage < 20%
```

---

## Timeline

- **30 min**: Find offsets with Cheat Engine
- **2 min**: Copy offsets to code
- **10 min**: Build project
- **5 min**: Run and test

**Total: ~50 minutes** to first visual

---

## Still Not Working?

1. **Read**: [RUNNING_IN_GAME.md](RUNNING_IN_GAME.md) - Full troubleshooting
2. **Check**: Were offsets correctly verified in Cheat Engine?
3. **Verify**: Is admin access granted? (try right-click → Run as Administrator)
4. **Debug**: Enable debug output and check console

---

## For Learning More

- **Architecture**: [FRAMEWORK_DESIGN.md](FRAMEWORK_DESIGN.md)
- **Implementation**: [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)
- **Diagrams**: [ARCHITECTURE.md](ARCHITECTURE.md)

---

**Remember:** The offsets are the most important part. If they're wrong, nothing will work!
