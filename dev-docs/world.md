# Game World Representation

This document describes how Fallout 2 represents and manages the game world. It serves as a reference for implementing accessibility features such as tile-by-tile exploration and nearby object scanning.

## Map and Tile System

The game uses an **isometric hex grid** for each map area.

### Grid Specifications

```cpp
// From map_defs.h
#define ELEVATION_COUNT (3)           // 3 vertical levels per map
#define HEX_GRID_WIDTH (200)          // Grid width in hexes
#define HEX_GRID_HEIGHT (200)         // Grid height in hexes
#define HEX_GRID_SIZE (40000)         // Total tiles (200 * 200)
```

### Key Files

| File | Purpose |
|------|---------|
| `src/map.cc`, `src/map.h` | Map loading, elevation management, global map state |
| `src/tile.cc`, `src/tile.h` | Tile coordinate conversion, hex grid math, distance calculations |
| `src/map_defs.h` | Grid dimensions and validation helpers |

### Tile Navigation

Tiles are numbered 0-39,999. The hex grid uses 6 directions:

```cpp
typedef enum Rotation {
    ROTATION_NE = 0,  // Northeast
    ROTATION_E = 1,   // East
    ROTATION_SE = 2,  // Southeast
    ROTATION_SW = 3,  // Southwest
    ROTATION_W = 4,   // West
    ROTATION_NW = 5,  // Northwest
    ROTATION_COUNT = 6
} Rotation;
```

**Key Functions:**

- `tileGetTileInDirection(tile, rotation, distance)` - Move N hexes in a direction (tile.cc:893)
- `tileDistanceBetween(tile1, tile2)` - Calculate hex distance between tiles (tile.cc:797)
- `tileGetRotationTo(tile1, tile2)` - Get direction from one tile toward another (tile.cc:910)
- `hexGridTileIsValid(tile)` - Check if tile index is valid (map_defs.h)

---

## Object Storage and Spatial Organization

### Primary Data Structure

Objects are stored in a **per-tile linked list** array:

```cpp
// From object.cc
static ObjectListNode* gObjectListHeadByTile[HEX_GRID_SIZE];  // 40,000 entries

typedef struct ObjectListNode {
    Object* obj;
    struct ObjectListNode* next;
} ObjectListNode;
```

Each tile (0-39,999) has a linked list of objects occupying it. Objects also track their elevation separately.

### Object Structure

```cpp
// From obj_types.h (simplified)
typedef struct Object {
    int id;           // Unique object ID
    int tile;         // Current hex tile (0-39,999)
    int elevation;    // Current elevation level (0-2)
    int rotation;     // Facing direction (0-5)
    int fid;          // Frame ID (art/animation)
    int flags;        // Behavior flags
    int pid;          // Prototype ID
    ObjectData data;  // Type-specific data
    Object* owner;    // Parent (for inventory items)
} Object;
```

### Object Types

```cpp
OBJ_TYPE_ITEM      // Weapons, ammo, consumables, misc items
OBJ_TYPE_CRITTER   // NPCs and enemies
OBJ_TYPE_SCENERY   // Doors, containers, stairs, ladders
OBJ_TYPE_WALL      // Walls and barriers
OBJ_TYPE_TILE      // Floor tiles
OBJ_TYPE_MISC      // Miscellaneous objects
```

Extract object type from FID: `FID_TYPE(object->fid)`

### Important Object Flags

```cpp
OBJECT_HIDDEN     = 0x01       // Object invisible
OBJECT_NO_BLOCK   = 0x10       // Doesn't block movement
OBJECT_MULTIHEX   = 0x800      // Large object spans multiple tiles
OBJECT_SEEN       = 0x40000000 // Discovered by player
```

### Key Files

| File | Purpose |
|------|---------|
| `src/object.cc`, `src/object.h` | Object creation, movement, spatial queries |
| `src/obj_types.h` | Object and critter structures, flags |
| `src/proto_types.h` | Prototype definitions, scenery subtypes |

---

## Object Query Functions

### Finding Objects at a Specific Tile

**Built-in functions (object.cc):**

```cpp
// Find first visible object at tile
Object* objectFindFirstAtLocation(int elevation, int tile);  // line 2254

// Get next object at same location (call repeatedly after First)
Object* objectFindNextAtLocation();  // line 2276
```

**Usage:**
```cpp
Object* obj = objectFindFirstAtLocation(gElevation, tile);
while (obj != nullptr) {
    // Process object
    obj = objectFindNextAtLocation();
}
```

### Finding Objects by Type

```cpp
// Get all objects of a type on the map (or at specific tile)
// Pass tile=-1 to search entire map
int objectListCreate(int tile, int elevation, int objectType, Object*** objectListPtr);

// Free the returned array
void objectListFree(Object** objectList);
```

**Usage:**
```cpp
Object** critters;
int count = objectListCreate(-1, gElevation, OBJ_TYPE_CRITTER, &critters);
for (int i = 0; i < count; i++) {
    Object* npc = critters[i];
    // Process NPC
}
objectListFree(critters);
```

### Blocking Object Detection

```cpp
// Check if tile is blocked (returns blocking object or nullptr)
Object* _obj_blocking_at(Object* excludeObj, int tile, int elevation);
```

### Direct Tile Access Pattern

For safe iteration without global state issues (see warning below):

```cpp
ObjectListNode* node = gObjectListHeadByTile[tile];
while (node != nullptr) {
    Object* obj = node->obj;
    if (obj->elevation == elevation && !(obj->flags & OBJECT_HIDDEN)) {
        // Process object - safe to call other functions here
    }
    node = node->next;
}
```

---

## Elevation System

### Overview

Each map has up to 3 elevation levels representing different floors:

- **Elevation 0** - Ground level / main floor
- **Elevation 1** - Basement, underground, or second floor
- **Elevation 2** - Third floor / deeper underground

All elevations share the same 200x200 tile grid but store objects independently.

### Key Variables

```cpp
extern Object* gDude;    // Player character object
extern int gElevation;   // Currently active/rendered elevation

// Player position
gDude->tile       // Current tile (0-39,999)
gDude->elevation  // Current elevation (0-2)
```

### Changing Elevation

Players move between elevations via scenery objects:

```cpp
SCENERY_TYPE_STAIRS       // Generic stairs
SCENERY_TYPE_LADDER_UP    // Ladder going up
SCENERY_TYPE_LADDER_DOWN  // Ladder going down
```

Elevation change is handled by `mapSetElevation(int elevation)` in map.cc:379.

### Query Filtering

All object queries should filter by elevation - objects on other floors won't be relevant:

```cpp
// Combat only considers critters on current elevation
objectListCreate(-1, gElevation, OBJ_TYPE_CRITTER, &combat_list);
```

---

## Distance and Direction

### Tile Distance

```cpp
int tileDistanceBetween(int tile1, int tile2);  // tile.cc:797
// Returns hex grid distance, 9999 if invalid
```

### Object Distance

```cpp
int objectGetDistanceBetween(Object* obj1, Object* obj2);  // object.cc
// Accounts for MULTIHEX objects (large creatures)
```

### Direction Between Tiles

```cpp
int tileGetRotationTo(int fromTile, int toTile);  // tile.cc:910
// Returns direction (0-5) from one tile toward another
```

### Adjacent Tile Iteration

```cpp
// Check all 6 neighbors
for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
    int adjacentTile = tileGetTileInDirection(tile, rotation, 1);
    if (hexGridTileIsValid(adjacentTile)) {
        // Process adjacent tile
    }
}
```

---

## Critical Warning: Non-Reentrant Iterator Pattern

### The Problem

`objectFindFirstAtLocation()` and `objectFindNextAtLocation()` use **global state**:

```cpp
static ObjectListNode* gObjectFindLastObjectListNode = nullptr;
static int gObjectFindElevation;
static int gObjectFindTile;
```

This means **nested calls will corrupt iteration state**:

```cpp
// BROKEN - inner loop destroys outer loop's state
Object* outer = objectFindFirstAtLocation(0, tile1);
while (outer != nullptr) {
    Object* inner = objectFindFirstAtLocation(0, tile2);  // CORRUPTS outer!
    while (inner != nullptr) {
        inner = objectFindNextAtLocation();
    }
    outer = objectFindNextAtLocation();  // Uses wrong state
}
```

### Safe Alternatives

1. **Use `objectListCreate()`** - allocates array, no global state issues
2. **Direct tile access** - iterate `gObjectListHeadByTile[tile]` manually
3. **Complete iteration before calling other object functions** - if using the built-in iterators

---

## Accessibility Implementation Notes

### For Tile-by-Tile Exploration

Key data to announce per tile:
- Objects at tile (iterate `gObjectListHeadByTile[tile]`)
- Object types and names (from prototypes)
- Direction from player (`tileGetRotationTo(gDude->tile, targetTile)`)
- Distance from player (`tileDistanceBetween(gDude->tile, targetTile)`)

### For Nearby Object Scanning

Approach options:
1. **Adjacent tiles only** - iterate 6 neighbors using `tileGetTileInDirection()`
2. **Radius search** - spiral out ring-by-ring (see combat.cc:4021 for pattern)
3. **All objects of type** - use `objectListCreate()`, sort by distance

Key interactable scenery types:
```cpp
SCENERY_TYPE_DOOR
SCENERY_TYPE_STAIRS
SCENERY_TYPE_LADDER_UP
SCENERY_TYPE_LADDER_DOWN
SCENERY_TYPE_GENERIC  // Containers, etc.
```

### Elevation Awareness

- Always filter queries by `gElevation`
- Announce elevation changes when player uses stairs/ladders
- Consider hooking `mapSetElevation()` for announcements
