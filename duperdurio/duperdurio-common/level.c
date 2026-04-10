#include "level.h"
#include <stddef.h>

void level_init(Level *lvl, uint8_t *tiles, int width, int height) {
    lvl->tiles           = tiles;
    lvl->width           = width;
    lvl->height          = height;
    lvl->cam_x           = 0;
    lvl->base_block_idx  = 0;
}

uint8_t level_tile(const Level *lvl, int tx, int ty) {
    int buf_tx = tx - lvl->base_block_idx * BLOCK_W;
    if (buf_tx < 0 || buf_tx >= lvl->width || ty < 0 || ty >= lvl->height)
        return TILE_AIR;
    return lvl->tiles[ty * lvl->width + buf_tx];
}

void level_set_tile(Level *lvl, int tx, int ty, uint8_t type) {
    int buf_tx = tx - lvl->base_block_idx * BLOCK_W;
    if (buf_tx < 0 || buf_tx >= lvl->width || ty < 0 || ty >= lvl->height) return;
    lvl->tiles[ty * lvl->width + buf_tx] = type;
}

int tile_is_solid(uint8_t type) {
    switch (type) {
    case TILE_GROUND:
    case TILE_BRICK:
    case TILE_QBLOCK:
    case TILE_QBLOCK_USED:
    case TILE_PIPE_TOP:
    case TILE_PIPE_BODY:
    case TILE_SOLID:
    case TILE_SHELL:
        return 1;
    default:
        return 0;
    }
}

int tile_is_brick(uint8_t type)  { return type == TILE_BRICK; }
int tile_is_qblock(uint8_t type) { return type == TILE_QBLOCK; }
