#ifndef FALLOUT_TILE_EXPLORER_H_
#define FALLOUT_TILE_EXPLORER_H_

namespace fallout {

// Initialize tile explorer module
void tileExplorerInit();

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

// Set the exploration cursor to a specific tile
void tileExplorerSetCursorTile(int tile);

// Calculate 8-way cardinal direction from tile1 to tile2 for announcements.
// Returns 0-7: N=0, NE=1, E=2, SE=3, S=4, SW=5, W=6, NW=7
// Returns -1 if tiles are the same or invalid.
int tileGetCardinalDirectionTo(int tile1, int tile2);

} // namespace fallout

#endif /* FALLOUT_TILE_EXPLORER_H_ */
