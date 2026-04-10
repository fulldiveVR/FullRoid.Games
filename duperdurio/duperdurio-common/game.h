#pragma once
#include "config.h"
#include "durio.h"
#include "enemy.h"
#include "nut.h"
#include "level.h"
#include "map_gen.h"
#include "save.h"
#include <stdint.h>

typedef enum {
    GAME_TITLE,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_DEAD
} GameState;

typedef struct {
    GameState state;

    Durio    durio;
    Enemy    enemies[MAX_ENEMIES];
    Nut      nuts[MAX_NUTS];
    Level    level;

    /* Infinite map: 3-block ring buffer */
    uint8_t  map_tiles[MAP_BUF_W * LEVEL_MAX_H]; /* 150×15 = 2250 bytes */
    int      base_block_idx;  /* world index of leftmost block in buffer */
    int      prev_pattern;    /* pattern index of block behind current */
    int      curr_pattern;    /* pattern index of current block */
    int      next_pattern;    /* pattern index of block ahead */

    /* Back-scroll tracking */
    int32_t  max_player_x_fp; /* furthest world pixel reached (fixed-point) */

    SaveData save;

    int      dead_wait;       /* ms to wait before allowing restart after death */
    int      respawn_timer;   /* ms after death before reload */
} Game;

/* Initialise/reset entire game (title screen) */
void game_init(Game *g);

/* Start a new game run from world 1-1 */
void game_start(Game *g);

/* Per-frame update: physics, enemies, coins, state transitions */
void game_tick(Game *g, int delta_ms);

/* Handle input — called after game_tick each frame */
void game_input(Game *g, int left, int right, int run,
                int jump_down, int jump_held,
                int pause, int confirm, int back, int delta_ms);
