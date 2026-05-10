# Complete Build & Run Workflow

Visual step-by-step guide with commands.

## Step 1: Prepare (Find Offsets)

### 1.1 Download Tools
```
Download Cheat Engine from: https://www.cheatengine.org/
Make sure CS2 is installed
```

### 1.2 Find Offsets
```
Follow: ~/Documents/csgocheats/FIND_OFFSETS.md

Target offsets to find:
  □ ENTITY_LIST
  □ LOCAL_PLAYER
  □ PLAYER_POSITION
  □ PLAYER_HEALTH
  □ PLAYER_TEAM
  □ PLAYER_BONE_MATRIX
```

### 1.3 Record Offsets
```
Write down 6 offset values (in hex):
  ENTITY_LIST:        0x________________
  LOCAL_PLAYER:       0x________________
  PLAYER_POSITION:    0x________________
  PLAYER_HEALTH:      0x________________
  PLAYER_TEAM:        0x________________
  PLAYER_BONE_MATRIX: 0x________________
```

## Step 2: Update Code

### 2.1 Open File
```bash
# Open in your text editor
~/Documents/csgocheats/include/game/game_structures.hpp
```

### 2.2 Replace Offsets
```cpp
// Find this section and replace with YOUR values:

namespace offsets {
    // Entity list offsets
    constexpr uintptr_t ENTITY_LIST = 0x1A8C690;      // ← Replace with YOUR value
    constexpr uintptr_t LOCAL_PLAYER = 0x1A8A6D8;     // ← Replace with YOUR value
    
    // Player data offsets
    constexpr uintptr_t PLAYER_POSITION = 0x1344;     // ← Replace with YOUR value
    constexpr uintptr_t PLAYER_HEALTH = 0x324;        // ← Replace with YOUR value
    constexpr uintptr_t PLAYER_TEAM = 0x3C0;          // ← Replace with YOUR value
    constexpr uintptr_t PLAYER_NAME = 0x588;
    constexpr uintptr_t PLAYER_BONE_MATRIX = 0x1080;
    
    // ... rest stays the same
};
```

### 2.3 Save File
```
Ctrl+S (save)
```

## Step 3: Build Project

### 3.1 Open Terminal
```
Option A: Command Prompt
  - Press Win+R
  - Type: cmd
  - Press Enter

Option B: PowerShell
  - Right-click → Open PowerShell

Option C: Visual Studio Code
  - Ctrl+` (backtick)
  - New terminal
```

### 3.2 Navigate to Project
```bash
cd Documents/csgocheats
```

### 3.3 Create Build Directory
```bash
mkdir build
cd build
```

### 3.4 Configure Project
```bash
# Option A: Auto-detect (recommended)
cmake ..

# Option B: Specify Visual Studio 2022
cmake -G "Visual Studio 17 2022" -A x64 ..

# Option C: Specify Visual Studio 2019
cmake -G "Visual Studio 16 2019" -A x64 ..
```

Output should show:
```
-- Configuring done
-- Generating done
-- Build files have been written to: ...
```

### 3.5 Build
```bash
# Debug build (faster compilation)
cmake --build . --config Debug

# Release build (optimized, smaller exe)
cmake --build . --config Release
```

Wait for it to finish. You should see:
```
Build files have been written to: ...
[100%] Built target cs2overlay
```

### 3.6 Verify Build Success
```bash
# Check if executable was created
ls Release/cs2overlay.exe

# Should show file details
# If "not found", something went wrong - check error messages above
```

## Step 4: Run Overlay

### 4.1 Prepare Game
```
1. Start Steam
2. Launch Counter-Strike 2
3. Wait for game to load
4. Join a Deathmatch or competitive game
5. Wait for game to fully load
6. Confirm you can see other players
```

### 4.2 Run Overlay (Method A: Command Line)
```bash
# From build directory
cd Release

# Run with admin (Windows will prompt for confirmation)
cs2overlay.exe
```

### 4.3 Run Overlay (Method B: GUI)
```
1. Open File Explorer
2. Navigate to: Documents\csgocheats\build\Release\
3. Right-click on: cs2overlay.exe
4. Select: "Run as Administrator"
5. Click: "Yes" when prompted
```

### 4.4 Verify Overlay Started
```
Look for:
  ✅ "CS2 Overlay" window appears
  ✅ Window is transparent (black = see-through)
  ✅ Window stays on top of game
```

## Step 5: Test Overlay

### 5.1 Check Player Rendering
```
In-game, look for:
  ✅ Green boxes around visible enemies
  ✅ Health bars above each player
  ✅ Boxes update when players move
  ✅ Health bars change when damage taken
```

### 5.2 Full Verification
```
Run through this checklist:

□ Overlay window visible
□ Window title says "CS2 Overlay"
□ Can see game through window (transparent black areas)
□ Green boxes around at least 1 player
□ Health bars visible above players
□ Health decreases when taking damage
□ Overlay stays responsive
□ No crashes after 5 minutes
```

## Troubleshooting During Build

### Problem: CMake not found
```
Solution:
  1. Install CMake from: https://cmake.org/download/
  2. Add to PATH (installation option)
  3. Restart terminal
  4. Try again: cmake --version
```

### Problem: Visual Studio not found
```
Solution:
  1. Install Visual Studio 2022 Community (free)
  2. Include "Desktop development with C++"
  3. Restart terminal
  4. Try: cmake -G "Visual Studio 17 2022" -A x64 ..
```

### Problem: Build fails with errors
```
Solution:
  1. Check CMakeLists.txt is in csgocheats/ folder
  2. Make sure you're in build/ folder
  3. Try: cmake .. (not from parent)
  4. Check full error message
  5. See IMPLEMENTATION_GUIDE.md if persists
```

### Problem: Build succeeds but no exe file
```
Solution:
  1. Check Release folder: ls Release/
  2. Check Debug folder: ls Debug/
  3. Rebuild: cmake --build . --config Release --verbose
  4. Look for compilation errors
```

## Troubleshooting During Run

### Problem: "This app cannot run on your PC"
```
Solution:
  DirectX 11 not available (very rare)
  
  Check:
  1. Windows 10+ installed
  2. Run: dxdiag.exe
  3. Check Display tab → DirectX version
  4. Should be 11.0 or higher
```

### Problem: "Permission denied" or "Access denied"
```
Solution:
  Not running as administrator
  
  Fix:
  1. Right-click cs2overlay.exe
  2. Select "Run as Administrator"
  3. Click "Yes" when asked
```

### Problem: No overlay window appears
```
Solution:
  Likely causes (in order):
  
  1. Not running as Administrator
     → Retry with admin privileges
     
  2. Game not running
     → Start CS2 first
     
  3. Memory offsets wrong
     → Re-verify with Cheat Engine
     → Check FIND_OFFSETS.md
     
  4. DirectX 11 issue
     → Check dxdiag.exe
     → Reinstall graphics drivers
```

### Problem: Window appears but no players showing
```
Solution:
  Offsets are wrong
  
  Steps to fix:
  1. Close overlay
  2. Re-scan with Cheat Engine (FIND_OFFSETS.md)
  3. Compare values - likely ENTITY_LIST or PLAYER_HEALTH
  4. Update game_structures.hpp
  5. Rebuild: cmake --build . --config Release
  6. Re-run overlay
```

### Problem: High CPU usage (>50%)
```
Solution:
  Memory reading too frequent
  
  Fix in src/main.cpp:
  
  // Decrease update frequency
  config.entity_config.update_interval_ms = 33.0f;  // 30 FPS instead
  
  // Then rebuild and re-run
```

## Troubleshooting After Running

### Issue: Overlay crashes on alt+tab
```
Solution:
  Check window handle validity in dx11_renderer.cpp
  (Likely edge case with window recreation)
```

### Issue: Boxes appear but in wrong positions
```
Solution:
  PLAYER_POSITION offset is wrong
  
  Steps:
  1. Close overlay
  2. Re-scan position offset with Cheat Engine
  3. Update game_structures.hpp
  4. Rebuild and test
```

### Issue: Health bars don't update
```
Solution:
  PLAYER_HEALTH offset is wrong
  
  Steps:
  1. Close overlay
  2. Re-scan health offset with Cheat Engine
  3. Verify it changes 100 → 80 → 60 etc when taking damage
  4. Update game_structures.hpp
  5. Rebuild and test
```

## Complete Command Sequence (Copy-Paste)

```bash
# Step 1: Navigate to project
cd ~/Documents/csgocheats

# Step 2: Create build directory
mkdir build && cd build

# Step 3: Configure
cmake ..

# Step 4: Build
cmake --build . --config Release

# Step 5: Verify build
ls Release/cs2overlay.exe

# Step 6: Run (from Release folder)
cd Release
cs2overlay.exe
```

## Quick Checklist

```
Before Building:
  ☐ Offsets found and verified
  ☐ game_structures.hpp updated
  ☐ CMake installed
  ☐ Visual Studio installed

Building:
  ☐ mkdir build && cd build
  ☐ cmake ..
  ☐ cmake --build . --config Release
  ☐ cs2overlay.exe exists

Running:
  ☐ CS2 running with players visible
  ☐ Right-click cs2overlay.exe
  ☐ "Run as Administrator"
  ☐ Overlay window appears

Verifying:
  ☐ Green boxes visible
  ☐ Health bars update
  ☐ No crashes
  ☐ Responsive
```

## Time Estimates

```
Find offsets:        30 min (first time)
Update code:          2 min
Configure + Build:   10 min
Run overlay:          5 min
Test & verify:        5 min
─────────────────────────
Total:              52 minutes (first time)
```

## Success Looks Like

```
Terminal output:
  [100%] Built target cs2overlay
  
  Overlay window:
  - Title: "CS2 Overlay"
  - Transparent background
  - Green boxes on players
  - Health bars visible
  - Real-time updates
  
  Performance:
  - 60+ FPS
  - <20% CPU
  - Responsive
```

---

**Ready?** → Start with Step 1: Prepare
**Need help?** → Check relevant Troubleshooting section above
**Still stuck?** → See `RUNNING_IN_GAME.md` for detailed help
