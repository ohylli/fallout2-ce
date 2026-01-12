#include "tile_explorer.h"

#include <stdio.h>
#include <string.h>

#include "art.h"
#include "debug.h"
#include "game_mouse.h"
#include "geometry.h"
#include "map.h"
#include "object.h"
#include "obj_types.h"
#include "tile.h"
#include "tolk.h"

namespace fallout {

// Exploration cursor tile position (-1 means cursor is at player position)
static int gTileExplorerCursorTile = -1;

// Visual cursor object for showing exploration position on screen
static Object* gTileExplorerVisualCursor = nullptr;

// Maximum number of objects to announce before saying "and more"
static const int kMaxAnnouncedObjects = 10;

// Direction names for announcements (indexed by ROTATION_NE..ROTATION_NW)
static const char* kDirectionNames[] = {
    "northeast",
    "east",
    "southeast",
    "southwest",
    "west",
    "northwest",
};

// Helper to create visual cursor (lazy initialization)
// Called only when cursor is first needed, after game systems are fully initialized
static void tileExplorerCreateVisualCursor()
{
    if (gTileExplorerVisualCursor != nullptr) {
        debugPrint("TileExplorer: cursor already exists\n");
        return; // Already created
    }

    debugPrint("TileExplorer: creating visual cursor\n");

    // Create visual cursor using hex cursor art (FID 1)
    int fid = buildFid(OBJ_TYPE_INTERFACE, 1, 0, 0, 0);
    int createResult = objectCreateWithFidPid(&gTileExplorerVisualCursor, fid, -1);
    debugPrint("TileExplorer: objectCreateWithFidPid returned %d, cursor=%p\n", createResult, gTileExplorerVisualCursor);

    if (createResult == 0) {
        // Set outline (same style as game mouse hex cursor)
        int outlineResult = objectSetOutline(gTileExplorerVisualCursor, OUTLINE_PALETTED | OUTLINE_TYPE_2, nullptr);
        debugPrint("TileExplorer: objectSetOutline returned %d\n", outlineResult);

        // Set flags to prevent interactions and saving
        gTileExplorerVisualCursor->flags |= (OBJECT_NO_REMOVE | OBJECT_NO_SAVE
            | OBJECT_LIGHT_THRU | OBJECT_SHOOT_THRU | OBJECT_NO_BLOCK);

        // Make it flat (rendered on ground)
        _obj_toggle_flat(gTileExplorerVisualCursor, nullptr);

        // Initially hidden
        Rect rect;
        objectHide(gTileExplorerVisualCursor, &rect);
        debugPrint("TileExplorer: cursor created and hidden, flags=0x%x, outline=0x%x\n",
            gTileExplorerVisualCursor->flags, gTileExplorerVisualCursor->outline);
    } else {
        debugPrint("TileExplorer: FAILED to create cursor object\n");
    }
}

// Helper to update visual cursor position and visibility
static void tileExplorerUpdateVisualCursor()
{
    debugPrint("TileExplorer: updateVisualCursor called, cursorTile=%d, gElevation=%d\n",
        gTileExplorerCursorTile, gElevation);

    // Lazy initialization: create cursor on first use (after game systems are ready)
    tileExplorerCreateVisualCursor();

    if (gTileExplorerVisualCursor == nullptr) {
        debugPrint("TileExplorer: cursor is NULL, aborting update\n");
        return;
    }

    Rect rect = { 0, 0, 0, 0 };
    if (gTileExplorerCursorTile == -1) {
        // Hide cursor when in "following player" mode
        debugPrint("TileExplorer: hiding cursor (tile=-1)\n");
        objectHide(gTileExplorerVisualCursor, &rect);
        tileWindowRefreshRect(&rect, gElevation);
    } else {
        // Show cursor at the exploration position
        debugPrint("TileExplorer: attempting to show cursor at tile %d, elevation %d\n",
            gTileExplorerCursorTile, gElevation);

        // Position the cursor (don't need the rect from this)
        int locResult = objectSetLocation(gTileExplorerVisualCursor, gTileExplorerCursorTile, gElevation, nullptr);
        debugPrint("TileExplorer: objectSetLocation returned %d\n", locResult);

        if (locResult == 0) {
            debugPrint("TileExplorer: cursor tile=%d, elevation=%d, flags=0x%x, outline=0x%x\n",
                gTileExplorerVisualCursor->tile, gTileExplorerVisualCursor->elevation,
                gTileExplorerVisualCursor->flags, gTileExplorerVisualCursor->outline);

            // Show the cursor and use this rect for refresh (like gameMouseObjectsShow does)
            Rect showRect;
            int showResult = objectShow(gTileExplorerVisualCursor, &showRect);
            debugPrint("TileExplorer: objectShow returned %d, showRect=(%d,%d,%d,%d)\n",
                showResult, showRect.left, showRect.top, showRect.right, showRect.bottom);

            if (showResult == 0) {
                objectEnableOutline(gTileExplorerVisualCursor, nullptr);

                debugPrint("TileExplorer: after show - flags=0x%x, outline=0x%x\n",
                    gTileExplorerVisualCursor->flags, gTileExplorerVisualCursor->outline);

                // Verify object is in the tile's object list
                bool foundInList = false;
                Object* obj = objectFindFirstAtLocation(gElevation, gTileExplorerCursorTile);
                while (obj != nullptr) {
                    if (obj == gTileExplorerVisualCursor) {
                        foundInList = true;
                        break;
                    }
                    obj = objectFindNextAtLocation();
                }
                debugPrint("TileExplorer: cursor in tile list: %s\n", foundInList ? "YES" : "NO");

                // Use showRect for refresh (same pattern as gameMouseObjectsShow)
                debugPrint("TileExplorer: calling tileWindowRefreshRect with showRect=(%d,%d,%d,%d)\n",
                    showRect.left, showRect.top, showRect.right, showRect.bottom);
                tileWindowRefreshRect(&showRect, gElevation);
            }
        } else {
            debugPrint("TileExplorer: objectSetLocation FAILED\n");
        }
    }
}

// Helper to get the effective exploration tile (cursor or player position)
static int tileExplorerGetEffectiveTile()
{
    if (gTileExplorerCursorTile != -1) {
        return gTileExplorerCursorTile;
    }
    if (gDude != nullptr) {
        return gDude->tile;
    }
    return -1;
}

void tileExplorerInit()
{
    // Clean up any existing cursor first to prevent memory leak on re-init
    if (gTileExplorerVisualCursor != nullptr) {
        tileExplorerExit();
    }

    gTileExplorerCursorTile = -1;
    // Visual cursor is created lazily in tileExplorerUpdateVisualCursor()
    // when first needed, after game systems are fully initialized
}

void tileExplorerExit()
{
    if (gTileExplorerVisualCursor != nullptr) {
        // Remove flags before destroying (consistent with game_mouse.cc pattern)
        gTileExplorerVisualCursor->flags &= ~(OBJECT_NO_SAVE | OBJECT_NO_REMOVE);
        objectDestroy(gTileExplorerVisualCursor, nullptr);
        gTileExplorerVisualCursor = nullptr;
    }
    gTileExplorerCursorTile = -1;
}

void tileExplorerResetToPlayer()
{
    debugPrint("TileExplorer: resetToPlayer called, gDude=%p\n", gDude);
    if (gDude != nullptr) {
        gTileExplorerCursorTile = gDude->tile;
        debugPrint("TileExplorer: set cursorTile to gDude->tile=%d\n", gDude->tile);
    } else {
        debugPrint("TileExplorer: gDude is NULL, cursorTile unchanged (%d)\n", gTileExplorerCursorTile);
    }
    tileExplorerUpdateVisualCursor();
}

bool tileExplorerMove(int direction)
{
    // Validate direction
    if (direction < 0 || direction >= ROTATION_COUNT) {
        return false;
    }

    // Get current tile
    int currentTile = tileExplorerGetEffectiveTile();
    if (currentTile == -1) {
        return false;
    }

    // Calculate new tile position
    int newTile = tileGetTileInDirection(currentTile, direction, 1);

    // Check if at map edge
    if (!tileIsValid(newTile) || tileIsEdge(newTile)) {
        return false;
    }

    // Update cursor position
    gTileExplorerCursorTile = newTile;

    // Update visual cursor
    tileExplorerUpdateVisualCursor();

    return true;
}

void tileExplorerAnnounceCurrentTile()
{
    int tile = tileExplorerGetEffectiveTile();
    if (tile == -1) {
        tolkSpeak("No position", true);
        return;
    }

    char announcement[1024];
    int pos = 0;
    int objectCount = 0;

    // Iterate through objects at this tile on current elevation
    Object* obj = objectFindFirstAtLocation(gElevation, tile);
    while (obj != nullptr) {
        // Skip the visual cursor itself (don't announce it to screen reader)
        if (obj == gTileExplorerVisualCursor) {
            obj = objectFindNextAtLocation();
            continue;
        }

        // Skip hidden objects
        if ((obj->flags & OBJECT_HIDDEN) == 0) {
            // Skip floor tiles (OBJ_TYPE_TILE) as they're not interesting
            int objType = FID_TYPE(obj->fid);
            if (objType != OBJ_TYPE_TILE) {
                char* name = objectGetName(obj);
                if (name != nullptr && name[0] != '\0') {
                    // Check if we have room for this name plus potential ", and more"
                    size_t nameLen = strlen(name);
                    size_t neededSpace = nameLen + (objectCount > 0 ? 2 : 0) + 15;

                    if (pos + neededSpace < sizeof(announcement)) {
                        if (objectCount > 0) {
                            pos += snprintf(announcement + pos, sizeof(announcement) - pos, ", ");
                        }
                        pos += snprintf(announcement + pos, sizeof(announcement) - pos, "%s", name);
                        objectCount++;
                    }

                    // Limit to reasonable number of objects
                    if (objectCount >= kMaxAnnouncedObjects) {
                        snprintf(announcement + pos, sizeof(announcement) - pos, ", and more");
                        break;
                    }
                }
            }
        }
        obj = objectFindNextAtLocation();
    }

    if (objectCount == 0) {
        tolkSpeak("Empty", true);
    } else {
        tolkSpeak(announcement, true);
    }
}

void tileExplorerAnnounceDistanceFromPlayer()
{
    if (gDude == nullptr) {
        tolkSpeak("No player position", true);
        return;
    }

    int cursorTile = gTileExplorerCursorTile;

    // If cursor not set, report at player position
    if (cursorTile == -1 || cursorTile == gDude->tile) {
        tolkSpeak("At player position", true);
        return;
    }

    // Calculate distance and direction from player to cursor
    int distance = tileDistanceBetween(gDude->tile, cursorTile);
    int direction = tileGetRotationTo(gDude->tile, cursorTile);

    // Bounds check for direction array
    if (direction < 0 || direction >= ROTATION_COUNT) {
        direction = ROTATION_NE;
    }

    char announcement[256];
    if (distance == 1) {
        snprintf(announcement, sizeof(announcement), "1 tile %s", kDirectionNames[direction]);
    } else {
        snprintf(announcement, sizeof(announcement), "%d tiles %s", distance, kDirectionNames[direction]);
    }

    tolkSpeak(announcement, true);
}

int tileExplorerGetCursorTile()
{
    return gTileExplorerCursorTile;
}

} // namespace fallout
