#pragma once
#include "config.h"
#include "enemy.h"
#include <stdint.h>

#define NUM_PATTERNS     21
#define MAX_PAT_ENEMIES  8

typedef struct {
    int       tx;    /* tile X within block, 0..BLOCK_W-1 */
    int       ty;    /* tile row for spawn formula (use 13 = ground row) */
    EnemyType type;
} PatternEnemy;

typedef struct {
    uint8_t      tiles[BLOCK_W * LEVEL_MAX_H];
    PatternEnemy enemies[MAX_PAT_ENEMIES];
    int          enemy_count;
} Pattern;

extern Pattern g_patterns[NUM_PATTERNS];

/* Call once at startup to fill all 21 patterns */
void mapgen_init_patterns(void);

/*
 * Copy pattern tiles into position buf_block (0, 1, or 2) inside dst.
 * dst must be MAP_BUF_W * LEVEL_MAX_H bytes (row-major, width=MAP_BUF_W).
 */
void mapgen_write_block(uint8_t *dst, int buf_block, const Pattern *pat);

/*
 * Return a random pattern index != excluded.
 * Pass -1 for excluded to allow any pattern.
 */
int mapgen_pick_next(int excluded);

/*
 * Fill the 3-block buffer from scratch (call at game_start).
 * Writes buf blocks 0, 1, 2 and returns their pattern indices via out_ids[3].
 */
void mapgen_init_buffer(uint8_t *dst, int out_ids[3]);
