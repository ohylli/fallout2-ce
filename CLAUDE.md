# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Fallout 2 Community Edition is a fully working re-implementation of Fallout 2 in
C++17 with the same original gameplay, engine bugfixes, and quality of life
improvements. It runs on Windows, Linux, macOS, Android, and iOS.

This is a fork of the orignal repository. Its aim is to make the game accessible
for blind screen reader users. See `accessibility.md` for details of the current status.

## Build Commands

### Windows (Visual Studio)
```bash
cmake --preset windows-x64
cmake --build --preset windows-x64-release
```

### Linux
```bash
# Install dependencies first: libsdl2-dev, zlib1g-dev
cmake --preset linux-x64-release
cmake --build --preset linux-x64-release
```

### macOS
```bash
cmake --preset macos
cmake --build --preset macos-release
```

### iOS
```bash
cmake --preset ios
cmake --build --preset ios-release
```

Build output is placed in `out/build/{preset-name}/`.

## Architecture

### Namespace
All code is in the `fallout` namespace.

### Core Systems

- **main.cc** - Entry point (`falloutMain`), main game loop, initialization/shutdown
- **game.cc/h** - Game state management, global variables, game mode tracking (`GameMode` class with flags like `kCombat`, `kDialog`, `kInventory`)
- **interpreter.cc/h** - Script VM that executes compiled game scripts (.INT files). Scripts are bytecode with opcodes starting at 0x8000. `Program` struct holds script state.
- **interpreter_extra.cc/h** - Game-specific script opcodes (functions callable from scripts)

### Data Layer

- **db.cc/h** - File I/O abstraction layer. Wraps file operations for .DAT archive support and cross-platform compatibility. Use `fileOpen`, `fileRead`, `fileClose` etc.
- **proto.cc/h** + **proto_types.h** - Prototype system defining game object templates (items, critters, scenery, walls, tiles). `Proto` union holds different prototype types.
- **object.cc/h** + **obj_types.h** - Runtime game objects. `Object` struct is the main game entity type. `gDude` is the player character.
- **cache.cc/h** - Art/asset caching system

### UI/Graphics

- **window_manager.cc/h** - Windowing system. `Window` and `Button` structs. Windows created with `windowCreate`, buttons with `buttonCreate`.
- **svga.cc/h** - SDL2-based rendering backend
- **art.cc/h** - FRM (frame) image loading and management
- **font_manager.cc/h**, **text_font.cc/h** - Text rendering

### Gameplay Systems

- **combat.cc/h** - Turn-based combat
- **combat_ai.cc/h** - NPC AI in combat
- **inventory.cc/h** - Item management
- **map.cc/h** - Map loading and tile management
- **worldmap.cc/h** - Travel between locations
- **scripts.cc/h** - Script attachment and execution
- **critter.cc/h** - Character stats, skills, damage

### Sfall Integration

Sfall compatibility layer (originally a Fallout mod) provides extended features:
- **sfall_config.cc/h** - Reads `ddraw.ini` configuration
- **sfall_opcodes.cc/h** - Extended script opcodes
- **sfall_arrays.cc/h** - Array data structures for scripts
- **sfall_global_scripts.cc/h** - Global script execution hooks
- **sfall_global_vars.cc/h** - Extended global variables

### Platform Abstraction

- **platform_compat.cc/h** - Cross-platform compatibility helpers
- **dinput.cc/h** - Input handling (keyboard/mouse)
- **audio_engine.cc/h** - SDL2 audio backend

### Mapper (Editor)

Located in `src/mapper/` - Map editor functionality, not part of main game.

## Configuration Files (Runtime)

- `fallout2.cfg` - Main game config (data paths, settings)
- `f2_res.ini` - Resolution and window settings (SCR_WIDTH, SCR_HEIGHT, WINDOWED)
- `ddraw.ini` - Sfall extended options

## Key Patterns

- Functions returning `int` typically return 0 for success, -1 for failure
- Many functions take `Rect* rect` as output parameter for dirty region tracking
- FID (Frame ID) combines object type, art index, and animation info using `buildFid()`
- PID (Prototype ID) encodes object type and proto index
- Game uses isometric hex grid with 3 elevations
