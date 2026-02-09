#include "tile_explorer.h"

#include <stdio.h>
#include <string.h>

#include "color.h"
#include "draw.h"
#include "geometry.h"
#include "map.h"
#include "object.h"
#include "tile.h"
#include "tolk.h"

namespace fallout {

// Exploration cursor tile position (-1 means cursor is not active)
static int gTileExplorerCursorTile = -1;

// Pre-rendered hex outline buffer (32x16 pixels, same size as tile grid)
static unsigned char gTileExplorerCursorBuf[32 * 16];

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

// Helper to refresh screen areas when cursor moves
static void tileExplorerRefreshCursorArea(int oldTile, int newTile, int elevation)
{
    int x, y;
    Rect r;

    // Refresh old position
    if (oldTile != -1) {
        tileToScreenXY(oldTile, &x, &y, elevation);
        r.left = x;
        r.top = y;
        r.right = x + 31;
        r.bottom = y + 15;
        tileWindowRefreshRect(&r, elevation);
    }

    // Refresh new position
    if (newTile != -1) {
        tileToScreenXY(newTile, &x, &y, elevation);
        r.left = x;
        r.top = y;
        r.right = x + 31;
        r.bottom = y + 15;
        tileWindowRefreshRect(&r, elevation);
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
    gTileExplorerCursorTile = -1;

    // Pre-render hex outline into cursor buffer
    // Outer hex (same vertices as _tile_grid_blocked in tile.cc)
    int color = _colorTable[31744]; // bright amber
    memset(gTileExplorerCursorBuf, 0, sizeof(gTileExplorerCursorBuf));
    bufferDrawLine(gTileExplorerCursorBuf, 32, 16, 0, 31, 4, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 31, 4, 31, 12, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 31, 12, 16, 15, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 0, 12, 16, 15, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 0, 4, 0, 12, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 16, 0, 0, 4, color);

    // Inner hex (1px inward for 2-pixel thickness / better visibility)
    bufferDrawLine(gTileExplorerCursorBuf, 32, 16, 1, 30, 4, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 30, 4, 30, 12, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 30, 12, 16, 14, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 1, 12, 16, 14, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 1, 4, 1, 12, color);
    bufferDrawLine(gTileExplorerCursorBuf, 32, 16, 1, 1, 4, color);
}

void tileExplorerExit()
{
    gTileExplorerCursorTile = -1;
}

void tileExplorerResetToPlayer()
{
    int oldTile = gTileExplorerCursorTile;

    if (gDude != nullptr) {
        gTileExplorerCursorTile = gDude->tile;
    }

    tileExplorerRefreshCursorArea(oldTile, gTileExplorerCursorTile, gElevation);
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

    // Save old tile for refresh
    int oldTile = gTileExplorerCursorTile;

    // Update cursor position
    gTileExplorerCursorTile = newTile;

    // Refresh old and new cursor areas
    tileExplorerRefreshCursorArea(oldTile, newTile, gElevation);

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

void tileExplorerRenderCursor(unsigned char* buf, int pitch, Rect* rect, int elevation)
{
    if (gTileExplorerCursorTile == -1) {
        return;
    }

    int x, y;
    tileToScreenXY(gTileExplorerCursorTile, &x, &y, elevation);

    Rect cursorRect;
    cursorRect.left = x;
    cursorRect.top = y;
    cursorRect.right = x + 31;
    cursorRect.bottom = y + 15;

    Rect clipped;
    if (rectIntersection(&cursorRect, rect, &clipped) == -1) {
        return;
    }

    blitBufferToBufferTrans(
        gTileExplorerCursorBuf + 32 * (clipped.top - y) + (clipped.left - x),
        clipped.right - clipped.left + 1,
        clipped.bottom - clipped.top + 1,
        32,
        buf + pitch * clipped.top + clipped.left,
        pitch);
}

int tileExplorerGetCursorTile()
{
    return gTileExplorerCursorTile;
}

} // namespace fallout
