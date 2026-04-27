# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Primary Build (Visual Studio - Recommended)
```bash
# Use the provided build script
build.bat
```

### Alternative Build Methods
```bash
# MinGW-w64
build_mingw.bat

# Direct compilation (VS)
compile.bat

# CMake
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Output**: `output/oi_simulator_gui.exe`

### Build Requirements
- Windows OS
- Visual Studio 2019+ or MinGW-w64
- DirectX 11 (included with Windows)
- C++17 standard

## Code Architecture

### Global State Architecture
The entire game operates on global variables defined in `game.hpp`. This is intentional to match the original console-based version's architecture. Key globals:
- `playerStats` - All player attributes (knowledge, abilities, determination, etc.)
- `mood` - Current mood level (0-12)
- `gameDifficulty` - Current difficulty setting
- `problems`, `subProblems` - Current contest problem state
- `gameLog` - Event log for UI display

### Header-Only Design
All core systems are implemented as header-only libraries:
- `types.hpp` - Type definitions and constants
- `game.hpp` - Game logic and utility functions
- `events.hpp` - Event system implementation
- `problem_pool.hpp` - Problem database (115 problems)
- `training_events_data.hpp` - Training event definitions

### Game Flow Architecture
The game follows a linear progression through `GuiScreen` states (defined in `main.cpp`):
1. **Home** - Main menu and difficulty selection
2. **Talent** - Initial talent point allocation
3. **Training** - Event-driven training phases (via `TRAINING_EVENTS` map)
4. **Notice** - Transition screens between phases
5. **Contest** - Problem-solving gameplay with think/code/check mechanics
6. **ContestResult** - Score display and awards
7. **GameOver** - End game statistics

### Core Systems

**Problem System (`problem_pool.hpp`)**
- Problems organized by difficulty level (1-7)
- Each problem has multiple sub-problems (`SubProblem` struct)
- Sub-problems define requirements (dp, ds, string, graph, combinatorics) and partial scoring

**Event System (`events.hpp`, `training_events_data.hpp`)**
- `TrainingEvent` struct with multiple `EventOption` choices
- Events can modify player stats, trigger other events, or lead to shop
- Shop system allows spending determination on stat boosts
- All events defined in `TRAINING_EVENTS` map in `training_events_data.hpp`
- **New Training Events**:
  - "步入高二" (Entering 11th Grade): Major storyline branching event
    - 专注OI: +500 determination, -2 culture
    - 均衡发展: +200 determination, +2 culture, +1 mood
    - 感到迷茫: -200 determination, -2 mood (may lead to anxiety)
    - 重整旗鼓: +300 determination, +1 thinking, +1 coding
- **Shop Expansion**:
  - New "经验提升" (Experience Boost) option
  - New "运气提升" (Luck Boost) option
  - Dynamic pricing: Costs increase after each purchase

**Contest System**
- Contest configurations in `CONTEST_CONFIGS` map (`types.hpp`)
- Progression: CSP-S → NOIP → WC → 省选 → APIO → NOI → CTT → CTS → IOI
- Each contest has specific problem difficulty ranges and time limits
- IOI contests use different scoring rules
- **Contest End Control**: Time reaching zero no longer auto-ends; requires "结束比赛" button click
- **IOI Special Format**: Zero-time submission capability preserved
- **Event Notifications**: Independent event popup pages during contests
- **Multi-Day Contests**: Special scoring handling for 省选, NOI, CTT, CTS, IOI

**Attribute System (`types.hpp`)**
- Knowledge: dp, ds, string, graph, combinatorics (0-20)
- Abilities: thinking, coding, carefulness, quickness, mental, experience, culture (0-20)
- Luck (运气): Reduces negative event probability (0-20, shop only)
- Determination (核心资源) - Used for shop purchases
- Mood (0-12) - Affects gameplay, impacts events
- TempExperience (临时经验): Accumulates to convert to permanent experience (6→1)

**Luck System (运气系统)**
- Location: `types.hpp` line 30, `game.hpp` lines 148-151
- Mechanism: Logarithmic formula `log2(luck + 1) * 0.095` calculates reduction rate
- Cap: Maximum 45% reduction of negative events (at luck=20)
- Acquisition: Shop purchase only, +1 per purchase, costs increase
- Effect: Reduces probability of negative random events during contests

**Code Modification System (修改代码机制)**
- Location: `game.hpp` lines 538-568, `main.cpp` lines 730-749, 1458-1481
- Trigger: Orange "修改代码" button appears after failed check (non-IOI contests)
- Time Cost: 1 + branch value
- Effect: First modification reduces error rate by 30%, subsequent mods provide no additional benefit
- Implementation Details:
  - `modificationCount[][]`: Tracks modification attempts per sub-problem
  - `hasAttemptedCheck[][]`: Requires re-checking after modification
  - Must re-check after modification to verify changes

**Experience & Blur System (经验与模糊系统)**
- Blur Levels (progressive disclosure based on think progress):
  - blur ≥ 3: Shows "模糊不清" (completely obscured)
  - blur = 2: Hides advanced requirements
  - blur = 1: Hides basic requirements
  - blur = 0: Shows all information
- Experience Effect: Each experience point reduces effective blur by 1
- Acquisition Methods:
  - Entering 11th grade: +1 permanent experience
  - Top awards in major contests: +1 temporary experience
  - Special training events
- Conversion: Every 6 tempExperience → 1 permanent experience

## Key Implementation Details

### ImGui Integration
- Uses DirectX 11 backend (`imgui_impl_dx11.cpp`)
- Win32 platform backend (`imgui_impl_win32.cpp`)
- Custom UI rendering in `main.cpp` with Chinese text support
- UTF-8 encoding handling via `/utf-8` flag (MSVC) or `-finput-charset` (MinGW)

### UI Enhancements
- **Character Status Panel**: Displays both permanent and temporary experience
- **Contest End Button**: Manual control over contest ending timing
- **Event Notification Pages**: Independent popup windows for event alerts
- **School Advancement Notice**: Dedicated page before entering 11th grade
- **Code Modification Button**: Orange button appears after failed checks (non-IOI)

### Problem-Solving Mechanics
- **Think Progress**: Must reach threshold to see full problem
- **Code Progress**: Must reach threshold to submit solution
- **Check Progress**: Optional verification phase
- Error rates calculated based on player stats vs problem requirements
- "Blur" mechanic shows `?` when think progress insufficient

### Stat Modification
- All stat changes go through `applyStatDelta()` in `events.hpp`
- Attributes clamped to [0, 20] range
- Mood capped at `MOOD_LIMIT` (12)
- Determination has no upper bound
- **Temporary Experience**: Accumulates, converts 6→1 to permanent experience
- **Luck**: Maximum 20, shop purchase only

### Logging
- `logEvent()` function writes to both `gameLog` vector and stdout
- Log types: "event", "think", "code", "check", "modify"
- Logs displayed in scrollable UI window during gameplay

## Development Notes

### Adding New Problems
Edit `problem_pool.hpp`, use `S()` macro for sub-problems:
```cpp
S(dp, ds, str, graph, comb, adhoc, thinking, coding, detail, trap, independent, heat, blur, branch, inspire, score)
```
- `branch` (分支): Code complexity, affects modification time cost (renamed from `fallback`)
- `inspire` (激励): Grants mood bonus when completing code
- `heat` (红温): Causes mood drop on failed attempts

### Adding New Training Events
Add entries to `TRAINING_EVENTS` map in `training_events_data.hpp`:
```cpp
{"事件名", {"标题", "描述", {
    {"选项文本", {{"stat", delta}}, {}, "", {}, {}, "", 0}
}, 选项数量, 是否商店}}
```

### Modifying Contests
Edit `CONTEST_CONFIGS` in `types.hpp` to adjust:
- Problem difficulty ranges
- Time limits
- IOI mode flag

### Unicode/Chinese Support
- MSVC: `/utf-8` flag automatically applied by build scripts
- MinGW: `-finput-charset=UTF-8 -fexec-charset=UTF-8`
- Source files must be UTF-8 encoded

## Project Context

This is a GUI remake of the original console-based OI simulator by Little09qwq. Key design differences:
- Real-time GUI instead of turn-based console
- Same game mechanics and progression
- Enhanced visual feedback for problem-solving
- Maintains the " deterministic + RNG" core gameplay loop

**Version**: v0.1.4-beta-2
**License**: Apache-2.0 (based on original MIT-licensed project)
