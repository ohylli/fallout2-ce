# Accessibility

This document tracks accessibility features for blind and visually impaired users.

## Screen Reader Support (Windows)

We use [Tolk](https://github.com/dkager/tolk) - a screen reader abstraction library that works with JAWS, NVDA, Window-Eyes, System Access, ZoomText, and Windows SAPI as fallback.

### Current Status

**Startup:**
- Tolk.dll loaded dynamically, announces "Fallout 2 loaded"
- Graceful degradation if Tolk.dll not present

**Main Menu:**
- Arrow keys (Up/Down) navigate with wraparound
- Enter activates selected option
- Announces "Main Menu" + current selection

**Character Selection (New Game):**
- Arrow keys (Left/Right) switch between 3 premade characters
- T key starts game with selected character
- M key modifies selected character, C creates new, B/ESC goes back
- Announces "Character Selection" + character name + biography on screen open
- Announces character name + biography when switching characters

**Tile Explorer (In-Game):**

Allows exploration of the game world tile by tile without moving the player character. The exploration cursor is independent from the player position and persists until reset.

| Key | Action |
|-----|--------|
| Shift+A | Move cursor West |
| Shift+W | Move cursor Northwest |
| Shift+E | Move cursor Northeast |
| Shift+D | Move cursor East |
| Shift+X | Move cursor Southeast |
| Shift+Z | Move cursor Southwest |
| F | Repeat current tile info (objects at cursor) |
| Shift+F | Announce distance and direction from player |
| Home | Return cursor to player position |

- Announces objects at each tile (items, NPCs, doors, containers, etc.)
- Says "Empty" for tiles with no interesting objects
- Says "Edge of map" when cursor reaches map boundary
- Works in all game states including combat
- Cursor stays in place when player moves (reset with Home key)

### Usage

1. Download Tolk from https://github.com/dkager/tolk/releases
2. Place `Tolk.dll` (x64) in the same directory as `fallout2-ce.exe`
3. Optionally copy screen reader API DLLs from Tolk's `lib/` folder
4. Run the game with a screen reader active (or SAPI will be used as fallback)

### API

```cpp
#include "tolk.h"

tolkInit();                           // Load Tolk.dll
tolkSpeak("Hello", true);             // Speak text (true = interrupt)
tolkIsActive();                       // Check if screen reader available
tolkExit();                           // Cleanup
```

### Files

- `src/tolk.h` - Public API
- `src/tolk.cc` - Implementation with dynamic DLL loading
- `src/mainmenu.cc` - Main menu keyboard navigation
- `src/character_selector.cc` - Character selection screen accessibility
- `src/tile_explorer.h` - Tile explorer public API
- `src/tile_explorer.cc` - Tile explorer implementation
- `src/game.cc` - Tile explorer key handlers (in `gameHandleKey()`)

### Implementation Notes

Pattern for adding keyboard navigation to menus:
1. Add static selection index variable
2. Handle `KEY_ARROW_UP`/`KEY_ARROW_DOWN` in event loop
3. Use `messageListGetItem()` to get UI text for announcements
4. Call `tolkSpeak(text, true)` with interrupt for immediate feedback

### Integration Points

Currently integrated in `src/game.cc`:
- `gameInitWithOptions()` - calls `tolkInit()`, `tolkSpeak()`, and `tileExplorerInit()`
- `gameHandleKey()` - handles tile explorer keyboard shortcuts
- `gameExit()` - calls `tolkExit()` during shutdown

## Future Work

- Nearby object scanner (find objects around player, integrate with tile explorer)
- Player movement via tile explorer (move player to explored tile)
- Object interaction via tile explorer (use/examine objects at cursor)
- Other menus (options, load/save, character creation)
- Dialog text reading
- Combat messages
- Inventory item descriptions
- Configuration option to enable/disable
