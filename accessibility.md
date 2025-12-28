# Accessibility

This document tracks accessibility features for blind and visually impaired users.

## Screen Reader Support (Windows)

We use [Tolk](https://github.com/dkager/tolk) - a screen reader abstraction library that works with JAWS, NVDA, Window-Eyes, System Access, ZoomText, and Windows SAPI as fallback.

### Current Status

Minimal proof of concept:
- Tolk.dll is loaded dynamically at startup
- Announces "Fallout 2 loaded" when the game starts
- Graceful degradation: game runs normally if Tolk.dll is not present

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

### Integration Points

Currently integrated in `src/game.cc`:
- `gameInitWithOptions()` - calls `tolkInit()` and `tolkSpeak()` after debug mode setup
- `gameExit()` - calls `tolkExit()` during shutdown

## Future Work

- Menu navigation announcements
- Dialog text reading
- Combat messages
- Inventory item descriptions
- Configuration option to enable/disable
