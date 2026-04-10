#pragma once
#include "config.h"
#include "physics.h"
#include <stdint.h>

typedef struct {
    int active;
    int32_t x_fp;   /* world pixels */
    int32_t y_fp;
    int32_t vy_fp;  /* popped nuts fly upward */
    int anim_timer; /* ms since spawned */
    int from_block; /* 1 = popped from ?-block (flies), 0 = static tile nut */
} Nut;

void nut_spawn(Nut *n, int wx, int wy, int from_block);
void nut_update(Nut *n, int delta_ms);

static inline Rect nut_hitbox(const Nut *n) {
    Rect r = { FP_TO_INT(n->x_fp) + 4, FP_TO_INT(n->y_fp) + 4, TILE_PX - 8, TILE_PX - 8 };
    return r;
}
