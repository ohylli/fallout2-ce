#ifndef FALLOUT_TILE_EXPLORER_H_
#define FALLOUT_TILE_EXPLORER_H_

#include "geometry.h"

namespace fallout {

// Initialize tile explorer module (pre-renders cursor hex outline)
void tileExplorerInit();

// Cleanup tile explorer module
void tileExplorerExit();

// Reset exploration cursor to player position
void tileExplorerResetToPlayer();

// Move exploration cursor in the specified hex direction (ROTATION_NE..ROTATION_NW)
// Returns true if moved successfully, false if at map edge
bool tileExplorerMove(int direction);

// Announce objects at the current exploration cursor tile
void tileExplorerAnnounceCurrentTile();

// Announce distance and direction from player to exploration cursor
void tileExplorerAnnounceDistanceFromPlayer();

// Render the exploration cursor hex outline into the tile window buffer
// Called from tileRefreshGame after _obj_render_post_roof
void tileExplorerRenderCursor(unsigned char* buf, int pitch, Rect* rect, int elevation);

// Get the current exploration cursor tile (-1 if not set)
int tileExplorerGetCursorTile();

} // namespace fallout

#endif /* FALLOUT_TILE_EXPLORER_H_ */
