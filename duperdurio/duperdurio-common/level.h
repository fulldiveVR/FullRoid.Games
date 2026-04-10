#pragma once
#include "config.h"
#include <stdint.h>

/*
 * Level layout: row-major uint8_t array [height][width].
 * Row 0 = top of level, row (height-1) = bottom (ground).
 * Camera scrolls the level left; Durio's screen X stays fixed near left third.
 */

typedef struct {
    uint8_t *tiles;          /* points to Game.map_tiles; writable */
    int      width;          /* = MAP_BUF_W = 150 (buffer width) */
    int      height;         /* = LEVEL_MAX_H = 15 */
    int      cam_x;          /* camera X in world pixels */
    int      base_block_idx; /* world index of the leftmost loaded block */
} Level;

/* Initialise level, pointing at an external tile buffer */
void level_init(Level *lvl, uint8_t *tiles, int width, int height);

/* Get tile at world tile grid (tx, ty). Returns TILE_AIR if out of buffer. */
uint8_t level_tile(const Level *lvl, int tx, int ty);

/* Set tile at world tile grid (tx, ty). */
void level_set_tile(Level *lvl, int tx, int ty, uint8_t type);

/* Convert world pixel X to tile column */
static inline int level_px_to_tx(int world_x) { return world_x / TILE_PX; }
/* Convert world pixel Y to tile row */
static inline int level_px_to_ty(int world_y) { return world_y / TILE_PX; }

/* World pixel origin of a tile */
static inline int level_tx_to_px(int tx) { return tx * TILE_PX; }
static inline int level_ty_to_px(int ty) { return ty * TILE_PX; }

/* Returns 1 if the tile is solid (blocks movement) */
int tile_is_solid(uint8_t type);

/* Returns 1 if the tile is a breakable brick */
int tile_is_brick(uint8_t type);

/* Returns 1 if the tile is an active ? block */
int tile_is_qblock(uint8_t type);

