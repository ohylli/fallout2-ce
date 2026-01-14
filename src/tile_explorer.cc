#include "tile_explorer.h"

#include <math.h>
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

// 8 cardinal direction names for announcements (N=0, NE=1, E=2, SE=3, S=4, SW=5, W=6, NW=7)
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

// Calculate 8-way cardinal direction from tile1 to tile2 for announcements.
// Returns 0-7: N=0, NE=1, E=2, SE=3, S=4, SW=5, W=6, NW=7
// Returns -1 if tiles are the same or invalid.
static int tileGetCardinalDirectionTo(int tile1, int tile2)
{
    if (tile1 == tile2) {
        return -1;
    }

    int x1, y1, x2, y2;
    if (tileToScreenXY(tile1, &x1, &y1, 0) != 0) {
        return -1;
    }
    if (tileToScreenXY(tile2, &x2, &y2, 0) != 0) {
        return -1;
    }

    // Calculate delta (invert Y for standard math coordinates where Y increases upward)
    double dx = static_cast<double>(x2 - x1);
    double dy = static_cast<double>(y1 - y2); // Inverted: screen Y down, math Y up

    // Handle edge case where both deltas are zero
    if (dx == 0.0 && dy == 0.0) {
        return -1;
    }

    // Calculate angle in radians using atan2
    // atan2 gives: East=0, North=PI/2, West=PI/-PI, South=-PI/2
    double angleRad = atan2(dy, dx);

    // Convert to degrees
    double angleDeg = angleRad * 180.0 / 3.14159265358979323846;

    // Transform so North=0: rotate 90 degrees
    // Standard atan2: E=0, N=90, W=180, S=-90
    // We want: N=0, E=90, S=180, W=270
    angleDeg = 90.0 - angleDeg;

    // Normalize to 0-360
    while (angleDeg < 0.0) {
        angleDeg += 360.0;
    }
    while (angleDeg >= 360.0) {
        angleDeg -= 360.0;
    }

    // Add 22.5 degrees (half sector) to center sectors on cardinal directions
    // This makes North cover 337.5-22.5 degrees
    angleDeg += 22.5;
    if (angleDeg >= 360.0) {
        angleDeg -= 360.0;
    }

    // Divide by 45 to get direction index 0-7
    int direction = static_cast<int>(angleDeg / 45.0);

    // Clamp to valid range (safety)
    if (direction < 0) direction = 0;
    if (direction > 7) direction = 7;

    return direction;
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
    int direction = tileGetCardinalDirectionTo(gDude->tile, cursorTile);

    // Should not happen since we checked tiles are different, but safety fallback
    if (direction < 0) {
        tolkSpeak("At player position", true);
        return;
    }

    char announcement[256];
    if (distance == 1) {
        snprintf(announcement, sizeof(announcement), "1 tile %s", kCardinalDirectionNames[direction]);
    } else {
        snprintf(announcement, sizeof(announcement), "%d tiles %s", distance, kCardinalDirectionNames[direction]);
    }

    tolkSpeak(announcement, true);
}

int tileExplorerGetCursorTile()
{
    return gTileExplorerCursorTile;
}

} // namespace fallout
