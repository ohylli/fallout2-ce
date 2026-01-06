#include "tile_explorer.h"

#include <stdio.h>
#include <string.h>

#include "art.h"
#include "map.h"
#include "object.h"
#include "obj_types.h"
#include "tile.h"
#include "tolk.h"

namespace fallout {

// Exploration cursor tile position (-1 means cursor is at player position)
static int gTileExplorerCursorTile = -1;

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
    gTileExplorerCursorTile = -1;
}

void tileExplorerResetToPlayer()
{
    if (gDude != nullptr) {
        gTileExplorerCursorTile = gDude->tile;
    }
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
