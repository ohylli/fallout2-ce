#include "object_scanner.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <vector>

#include "item.h"
#include "kb.h"
#include "map.h"
#include "object.h"
#include "obj_types.h"
#include "proto.h"
#include "proto_types.h"
#include "tile.h"
#include "tile_explorer.h"
#include "tolk.h"

namespace fallout {

enum ScannerCategory {
    SCANNER_CATEGORY_ALL,
    SCANNER_CATEGORY_CHARACTERS,
    SCANNER_CATEGORY_ITEMS,
    SCANNER_CATEGORY_ENTRANCES,
    SCANNER_CATEGORY_COUNT,
};

static const char* kCategoryNames[] = {
    "All",
    "Characters",
    "Items",
    "Entrances",
};

// Cardinal direction names matching tileGetCardinalDirectionTo indices
static const char* kCardinalDirectionNames[] = {
    "north",
    "northeast",
    "east",
    "southeast",
    "south",
    "southwest",
    "west",
    "northwest",
};

// Per-category object lists, sorted by distance to player
static std::vector<Object*> gCategoryObjects[SCANNER_CATEGORY_COUNT];

// Current index within each category
static int gCategoryIndex[SCANNER_CATEGORY_COUNT];

// Currently active category
static ScannerCategory gCurrentCategory = SCANNER_CATEGORY_ALL;

// Whether the scanner has been populated at least once
static bool gScannerPopulated = false;

// Map name and elevation at last scan, used to detect stale pointers
static char gScannedMapName[16] = "";
static int gScannedElevation = 0;

// Check if the scanned data is still valid (same map and elevation)
static bool isScanValid()
{
    if (!gScannerPopulated) {
        return false;
    }
    if (gScannedElevation != gElevation) {
        return false;
    }
    if (strcmp(gScannedMapName, gMapHeader.name) != 0) {
        return false;
    }
    return true;
}

// Determine which scanner category an object belongs to.
// Returns -1 if the object should not be included in the scanner.
static int classifyObject(Object* obj)
{
    if (obj == gDude) {
        return -1;
    }

    if ((obj->flags & OBJECT_HIDDEN) != 0) {
        return -1;
    }

    int objType = FID_TYPE(obj->fid);

    switch (objType) {
    case OBJ_TYPE_CRITTER:
        return SCANNER_CATEGORY_CHARACTERS;

    case OBJ_TYPE_ITEM:
        return SCANNER_CATEGORY_ITEMS;

    case OBJ_TYPE_SCENERY: {
        Proto* proto;
        if (protoGetProto(obj->pid, &proto) != 0) {
            return -1;
        }
        switch (proto->scenery.type) {
        case SCENERY_TYPE_DOOR:
        case SCENERY_TYPE_STAIRS:
        case SCENERY_TYPE_ELEVATOR:
        case SCENERY_TYPE_LADDER_UP:
        case SCENERY_TYPE_LADDER_DOWN:
            return SCANNER_CATEGORY_ENTRANCES;
        default:
            return -1;
        }
    }

    case OBJ_TYPE_MISC:
        if (isExitGridPid(obj->pid)) {
            return SCANNER_CATEGORY_ENTRANCES;
        }
        return -1;

    default:
        return -1;
    }
}

// Collect objects from one objectListCreate call into category lists
static void collectObjectsOfType(int objectType)
{
    Object** objectList = nullptr;
    int count = objectListCreate(-1, gElevation, objectType, &objectList);
    if (count <= 0 || objectList == nullptr) {
        return;
    }

    for (int i = 0; i < count; i++) {
        int category = classifyObject(objectList[i]);
        if (category >= 0) {
            gCategoryObjects[SCANNER_CATEGORY_ALL].push_back(objectList[i]);
            gCategoryObjects[category].push_back(objectList[i]);
        }
    }

    objectListFree(objectList);
}

// Sort a category list by distance to player
static void sortByDistance(std::vector<Object*>& objects)
{
    int playerTile = gDude->tile;
    std::sort(objects.begin(), objects.end(),
        [playerTile](Object* a, Object* b) {
            return tileDistanceBetween(playerTile, a->tile)
                < tileDistanceBetween(playerTile, b->tile);
        });
}

// Scan the entire map and populate category lists
static void scanObjects()
{
    for (int i = 0; i < SCANNER_CATEGORY_COUNT; i++) {
        gCategoryObjects[i].clear();
        gCategoryIndex[i] = 0;
    }

    if (gDude == nullptr) {
        return;
    }

    collectObjectsOfType(OBJ_TYPE_CRITTER);
    collectObjectsOfType(OBJ_TYPE_ITEM);
    collectObjectsOfType(OBJ_TYPE_SCENERY);
    collectObjectsOfType(OBJ_TYPE_MISC);

    for (int i = 0; i < SCANNER_CATEGORY_COUNT; i++) {
        sortByDistance(gCategoryObjects[i]);
    }

    strncpy(gScannedMapName, gMapHeader.name, sizeof(gScannedMapName) - 1);
    gScannedMapName[sizeof(gScannedMapName) - 1] = '\0';
    gScannedElevation = gElevation;
    gScannerPopulated = true;
}

// Format and speak an object announcement.
// If prefix is not null, it is prepended (e.g. category name or "Scanner refreshed").
static void announceObject(const char* prefix)
{
    std::vector<Object*>& objects = gCategoryObjects[gCurrentCategory];
    int index = gCategoryIndex[gCurrentCategory];

    if (objects.empty()) {
        const char* label = (prefix != nullptr) ? prefix : kCategoryNames[gCurrentCategory];
        char announcement[256];
        snprintf(announcement, sizeof(announcement), "%s: empty", label);
        tolkSpeak(announcement, true);
        return;
    }

    Object* obj = objects[index];
    char* name = objectGetName(obj);
    int distance = tileDistanceBetween(gDude->tile, obj->tile);
    int direction = tileGetCardinalDirectionTo(gDude->tile, obj->tile);
    int total = static_cast<int>(objects.size());
    int displayIndex = index + 1;

    // Build location part: "N tiles direction" or "at player"
    char location[64];
    if (direction >= 0) {
        snprintf(location, sizeof(location), "%d tile%s %s",
            distance, distance == 1 ? "" : "s", kCardinalDirectionNames[direction]);
    } else {
        snprintf(location, sizeof(location), "at player");
    }

    char announcement[512];
    if (prefix != nullptr) {
        snprintf(announcement, sizeof(announcement), "%s: %s, %s, %d of %d",
            prefix, name, location, displayIndex, total);
    } else {
        snprintf(announcement, sizeof(announcement), "%s, %s, %d of %d",
            name, location, displayIndex, total);
    }

    tolkSpeak(announcement, true);
}

// Navigate to the next or previous object in the current category
static void navigateObject(bool forward)
{
    if (!isScanValid()) {
        scanObjects();
    }

    std::vector<Object*>& objects = gCategoryObjects[gCurrentCategory];
    if (objects.empty()) {
        announceObject(nullptr);
        return;
    }

    int& index = gCategoryIndex[gCurrentCategory];
    if (forward) {
        index = (index + 1) % static_cast<int>(objects.size());
    } else {
        index = (index - 1 + static_cast<int>(objects.size())) % static_cast<int>(objects.size());
    }

    tileExplorerSetCursorTile(objects[index]->tile);
    announceObject(nullptr);
}

// Switch to the next or previous category
static void switchCategory(bool forward)
{
    if (!isScanValid()) {
        scanObjects();
    }

    if (forward) {
        gCurrentCategory = static_cast<ScannerCategory>(
            (gCurrentCategory + 1) % SCANNER_CATEGORY_COUNT);
    } else {
        gCurrentCategory = static_cast<ScannerCategory>(
            (gCurrentCategory - 1 + SCANNER_CATEGORY_COUNT) % SCANNER_CATEGORY_COUNT);
    }

    std::vector<Object*>& objects = gCategoryObjects[gCurrentCategory];
    int index = gCategoryIndex[gCurrentCategory];
    if (!objects.empty()) {
        tileExplorerSetCursorTile(objects[index]->tile);
    }

    announceObject(kCategoryNames[gCurrentCategory]);
}

// Refresh the scanner
static void refreshScanner()
{
    scanObjects();

    std::vector<Object*>& objects = gCategoryObjects[gCurrentCategory];
    if (!objects.empty()) {
        tileExplorerSetCursorTile(objects[0]->tile);
    }

    announceObject("Scanner refreshed");
}

void objectScannerInit()
{
    for (int i = 0; i < SCANNER_CATEGORY_COUNT; i++) {
        gCategoryObjects[i].clear();
        gCategoryIndex[i] = 0;
    }
    gCurrentCategory = SCANNER_CATEGORY_ALL;
    gScannerPopulated = false;
}

bool objectScannerHandleKey(int key)
{
    switch (key) {
    case KEY_PAGE_DOWN:
        navigateObject(true);
        return true;
    case KEY_PAGE_UP:
        navigateObject(false);
        return true;
    case KEY_CTRL_PAGE_DOWN:
        switchCategory(true);
        return true;
    case KEY_CTRL_PAGE_UP:
        switchCategory(false);
        return true;
    case KEY_END:
        refreshScanner();
        return true;
    default:
        return false;
    }
}

} // namespace fallout
