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

### Implementation Notes

Pattern for adding keyboard navigation to menus:
1. Add static selection index variable
2. Handle `KEY_ARROW_UP`/`KEY_ARROW_DOWN` in event loop
3. Use `messageListGetItem()` to get UI text for announcements
4. Call `tolkSpeak(text, true)` with interrupt for immediate feedback

### Integration Points

Currently integrated in `src/game.cc`:
- `gameInitWithOptions()` - calls `tolkInit()` and `tolkSpeak()` after debug mode setup
- `gameExit()` - calls `tolkExit()` during shutdown

## Future Work

- Other menus (options, load/save, character creation)
- Dialog text reading
- Combat messages
- Inventory item descriptions
- Configuration option to enable/disable
