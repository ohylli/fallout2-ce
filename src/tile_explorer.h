#ifndef FALLOUT_TILE_EXPLORER_H_
#define FALLOUT_TILE_EXPLORER_H_

namespace fallout {

// Initialize tile explorer module (creates visual cursor)
void tileExplorerInit();

// Cleanup tile explorer module (destroys visual cursor)
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

// Get the current exploration cursor tile (-1 if not set)
int tileExplorerGetCursorTile();

} // namespace fallout

#endif /* FALLOUT_TILE_EXPLORER_H_ */
