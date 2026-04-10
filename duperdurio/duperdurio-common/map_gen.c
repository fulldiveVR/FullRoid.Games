#include "map_gen.h"
#include "rng.h"
#include <string.h>

Pattern g_patterns[NUM_PATTERNS];

/* ─────────────────────────────────────────────────────────────
 * Pattern builder helpers
 * ───────────────────────────────────────────────────────────── */

static void pat_set(Pattern *p, int row, int col, uint8_t t) {
    if (row >= 0 && row < LEVEL_MAX_H && col >= 0 && col < BLOCK_W)
        p->tiles[row * BLOCK_W + col] = t;
}

static void pat_hline(Pattern *p, int row, int c0, int c1, uint8_t t) {
    for (int c = c0; c <= c1; c++) pat_set(p, row, c, t);
}

/* Solid ground on rows 13-14 across the whole block */
static void pat_ground(Pattern *p) {
    pat_hline(p, 13, 0, BLOCK_W - 1, TILE_GROUND);
    pat_hline(p, 14, 0, BLOCK_W - 1, TILE_GROUND);
}

/* Dig a pit (air) in rows 13-14 for columns c0..c1
   Caller must ensure c0 >= 4 and c1 <= BLOCK_W - 5. */
static void pat_pit(Pattern *p, int c0, int c1) {
    for (int c = c0; c <= c1; c++) {
        p->tiles[13 * BLOCK_W + c] = TILE_AIR;
        p->tiles[14 * BLOCK_W + c] = TILE_AIR;
    }
}

/* Pipe: height tiles tall, 2 tiles wide starting at col.
   top row = (13 - height), body fills down to row 13.
   Nuts (if nut_above != 0) placed one row above the pipe top. */
static void pat_pipe(Pattern *p, int col, int height) {
    int top_row = 13 - height;
    pat_set(p, top_row,     col,     TILE_PIPE_TOP);
    pat_set(p, top_row,     col + 1, TILE_PIPE_TOP);
    for (int r = top_row + 1; r <= 13; r++) {
        pat_set(p, r, col,     TILE_PIPE_BODY);
        pat_set(p, r, col + 1, TILE_PIPE_BODY);
    }
}

/* Place nuts one tile above a pipe of given height at given col */
static void pat_nuts_above_pipe(Pattern *p, int col, int height) {
    int above = 13 - height - 1;
    pat_set(p, above, col,     TILE_NUT);
    pat_set(p, above, col + 1, TILE_NUT);
}

/* Solid wall segment: rows r0..r1, cols c0..c1 */
static void pat_wall(Pattern *p, int r0, int r1, int c0, int c1, uint8_t t) {
    for (int r = r0; r <= r1; r++)
        pat_hline(p, r, c0, c1, t);
}

static void pat_add_enemy(Pattern *p, int tx, int ty, EnemyType type) {
    if (p->enemy_count < MAX_PAT_ENEMIES) {
        p->enemies[p->enemy_count].tx   = tx;
        p->enemies[p->enemy_count].ty   = ty;
        p->enemies[p->enemy_count].type = type;
        p->enemy_count++;
    }
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 0: flat_open
 * Open ground with a mid-air brick shelf and scattered enemies.
 * ───────────────────────────────────────────────────────────── */
static void init_pat0(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Brick shelf in the middle */
    pat_hline(p, 10, 20, 30, TILE_BRICK);
    pat_set(p, 10, 25, TILE_QBLOCK);
    /* ?-blocks in open air */
    pat_set(p, 8, 10, TILE_QBLOCK);
    pat_set(p, 8, 38, TILE_QBLOCK);
    /* Embedded nuts on ground level */
    pat_set(p, 9,  5, TILE_NUT);
    pat_set(p, 9, 44, TILE_NUT);
    pat_add_enemy(p,  8, 13, ENEMY_CRAB);
    pat_add_enemy(p, 32, 13, ENEMY_CRAB);
    pat_add_enemy(p, 44, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 1: pipe_trio
 * Three 2-tile pipes evenly spaced; crabs patrol the gaps.
 * ───────────────────────────────────────────────────────────── */
static void init_pat1(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_pipe(p,  7, 2);
    pat_pipe(p, 22, 2);
    pat_pipe(p, 37, 2);
    pat_nuts_above_pipe(p,  7, 2);
    pat_nuts_above_pipe(p, 22, 2);
    pat_nuts_above_pipe(p, 37, 2);
    pat_set(p, 8, 15, TILE_QBLOCK);
    pat_set(p, 8, 31, TILE_QBLOCK);
    pat_add_enemy(p, 15, 13, ENEMY_CRAB);
    pat_add_enemy(p, 30, 13, ENEMY_CRAB);
    pat_add_enemy(p, 44, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 2: staircase_up
 * 4-step ascending staircase; snail guards the exit.
 * ───────────────────────────────────────────────────────────── */
static void init_pat2(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Step 1 row 12, step 2 rows 11-12, step 3 rows 10-12, step 4 rows 9-12 */
    pat_hline(p, 12,  5,  7, TILE_SOLID);
    pat_wall (p, 11, 12,  8, 10, TILE_SOLID);
    pat_wall (p, 10, 12, 11, 13, TILE_SOLID);
    pat_wall (p,  9, 12, 14, 16, TILE_SOLID);
    pat_set(p, 7, 20, TILE_QBLOCK);   /* open air right of staircase */
    /* Nuts and enemies on the right side */
    pat_set(p, 9, 28, TILE_NUT);
    pat_set(p, 9, 38, TILE_NUT);
    pat_set(p, 8, 40, TILE_QBLOCK);
    pat_add_enemy(p, 25, 13, ENEMY_CRAB);
    pat_add_enemy(p, 35, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 44, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 3: staircase_down
 * 4-step descending staircase with enemies on both sides.
 * ───────────────────────────────────────────────────────────── */
static void init_pat3(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Steps descend left to right */
    pat_wall (p,  9, 12, 28, 30, TILE_SOLID);
    pat_wall (p, 10, 12, 31, 33, TILE_SOLID);
    pat_wall (p, 11, 12, 34, 36, TILE_SOLID);
    pat_hline(p, 12, 37, 39, TILE_SOLID);
    pat_set(p, 7, 23, TILE_QBLOCK);   /* open air before the descending stairs */
    /* Approach / exit content */
    pat_set(p, 9,  8, TILE_NUT);
    pat_set(p, 9, 43, TILE_NUT);
    pat_set(p, 8, 13, TILE_QBLOCK);
    pat_add_enemy(p,  5, 13, ENEMY_CRAB);
    pat_add_enemy(p, 18, 13, ENEMY_CRAB);
    pat_add_enemy(p, 43, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 4: pit_narrow
 * Single pit with platform above; enemies before and after.
 * ───────────────────────────────────────────────────────────── */
static void init_pat4(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_pit(p, 22, 24);
    pat_hline(p, 10, 20, 26, TILE_BRICK);
    pat_set(p, 8, 23, TILE_QBLOCK);
    pat_set(p, 8, 10, TILE_QBLOCK);
    pat_set(p, 8, 38, TILE_QBLOCK);
    pat_set(p, 9,  5, TILE_NUT);
    pat_set(p, 9, 44, TILE_NUT);
    pat_add_enemy(p, 10, 13, ENEMY_CRAB);
    pat_add_enemy(p, 34, 13, ENEMY_CRAB);
    pat_add_enemy(p, 44, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 5: pit_wide
 * Two 4-tile pits; stepping-stone over first pit; enemies in middle.
 * ───────────────────────────────────────────────────────────── */
static void init_pat5(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_pit(p,  8, 11);
    pat_pit(p, 33, 36);
    pat_hline(p, 9, 7, 12, TILE_BRICK);   /* stepping stone over pit 1 */
    pat_set(p, 7, 10, TILE_QBLOCK);
    pat_set(p, 8, 22, TILE_QBLOCK);
    pat_set(p, 7, 37, TILE_QBLOCK);
    pat_set(p, 9,  3, TILE_NUT);
    pat_set(p, 9, 43, TILE_NUT);
    pat_add_enemy(p, 20, 13, ENEMY_CRAB);
    pat_add_enemy(p, 25, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 43, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 6: brick_arch
 * Long brick arch with central ?-block; crabs bookend the arch.
 * ───────────────────────────────────────────────────────────── */
static void init_pat6(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_hline(p, 11, 15, 35, TILE_BRICK);
    pat_set(p, 9, 25, TILE_QBLOCK);
    /* Short brick platforms on both ends for variety */
    pat_hline(p, 10,  5,  9, TILE_BRICK);
    pat_hline(p, 10, 40, 44, TILE_BRICK);
    pat_set(p, 9,  7, TILE_NUT);
    pat_set(p, 9, 42, TILE_NUT);
    pat_set(p, 8, 20, TILE_QBLOCK);
    pat_set(p, 8, 30, TILE_QBLOCK);
    pat_add_enemy(p,  6, 13, ENEMY_CRAB);
    pat_add_enemy(p, 22, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 41, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 7: wall_step
 * Stepping stones give the boost needed to clear tall walls.
 * Each wall is 5 tiles high; the low platform before it lets
 * the player arc over comfortably.
 * ───────────────────────────────────────────────────────────── */
static void init_pat7(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Stepping stone 1: 3 tiles above ground */
    pat_hline(p, 10,  7, 12, TILE_SOLID);
    /* Wall 1: 5 tiles tall */
    pat_wall (p,  8, 12, 20, 22, TILE_SOLID);
    /* Stepping stone 2 before wall 2 */
    pat_hline(p, 10, 28, 33, TILE_SOLID);
    /* Wall 2: 5 tiles tall */
    pat_wall (p,  8, 12, 41, 43, TILE_SOLID);
    /* ?-blocks: above each stepping stone and in the middle gap */
    pat_set(p, 7, 10, TILE_QBLOCK);   /* hittable jumping from stone 1 */
    pat_set(p, 8, 25, TILE_QBLOCK);   /* hittable from ground in gap */
    pat_set(p, 7, 31, TILE_QBLOCK);   /* hittable jumping from stone 2 */
    pat_set(p, 9,  3, TILE_NUT);
    pat_set(p, 9, 46, TILE_NUT);
    pat_add_enemy(p,  4, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 16, 13, ENEMY_CRAB);
    pat_add_enemy(p, 34, 13, ENEMY_CRAB);
    pat_add_enemy(p, 46, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 8: brick_rows
 * Staggered brick platforms at two heights; enemies everywhere.
 * ───────────────────────────────────────────────────────────── */
static void init_pat8(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_hline(p, 11,  4, 15, TILE_BRICK);
    pat_hline(p,  9, 20, 36, TILE_BRICK);
    pat_hline(p, 11, 40, 47, TILE_BRICK);
    pat_set(p,  9, 10, TILE_QBLOCK);   /* moved from row 11: now 2 tiles above ground */
    pat_set(p,  9, 25, TILE_QBLOCK);
    pat_set(p,  9, 31, TILE_QBLOCK);
    pat_set(p,  9, 44, TILE_QBLOCK);   /* moved from row 11: now 2 tiles above ground */
    pat_set(p, 10, 18, TILE_NUT);
    pat_set(p,  8, 28, TILE_NUT);
    pat_add_enemy(p,  7, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 18, 13, ENEMY_CRAB);
    pat_add_enemy(p, 38, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 9: enemy_gauntlet
 * Four crabs on open ground; small platforms give breath room.
 * ───────────────────────────────────────────────────────────── */
static void init_pat9(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Two small refuge platforms */
    pat_hline(p, 9,  5,  9, TILE_BRICK);
    pat_hline(p, 9, 38, 44, TILE_BRICK);
    pat_set(p, 8,  7, TILE_QBLOCK);
    pat_set(p, 8, 16, TILE_QBLOCK);
    pat_set(p, 8, 32, TILE_QBLOCK);
    pat_set(p, 8, 41, TILE_QBLOCK);
    pat_set(p, 9, 24, TILE_NUT);
    pat_add_enemy(p, 13, 13, ENEMY_CRAB);
    pat_add_enemy(p, 21, 13, ENEMY_CRAB);
    pat_add_enemy(p, 29, 13, ENEMY_CRAB);
    pat_add_enemy(p, 44, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 10: pipe_maze
 * Three pipes of increasing height; crabs in every gap.
 * ───────────────────────────────────────────────────────────── */
static void init_pat10(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_pipe(p,  7, 2);
    pat_pipe(p, 21, 3);
    pat_pipe(p, 36, 4);
    pat_nuts_above_pipe(p,  7, 2);
    pat_nuts_above_pipe(p, 21, 3);
    pat_nuts_above_pipe(p, 36, 4);
    pat_set(p, 8, 30, TILE_QBLOCK);
    pat_set(p, 9,  3, TILE_NUT);
    pat_set(p, 9, 45, TILE_NUT);
    pat_add_enemy(p,  4, 13, ENEMY_CRAB);
    pat_add_enemy(p, 15, 13, ENEMY_CRAB);
    pat_add_enemy(p, 30, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 11: coin_heaven
 * Long sky platform loaded with nuts; snail guards ground below.
 * ───────────────────────────────────────────────────────────── */
static void init_pat11(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_hline(p, 7, 8, 42, TILE_BRICK);
    /* Dense nut row just above the platform */
    for (int c = 9; c <= 41; c += 2)
        pat_set(p, 6, c, TILE_NUT);
    /* Crab on the ground before the platform */
    pat_set(p, 9,  4, TILE_NUT);
    pat_set(p, 9, 45, TILE_NUT);
    pat_add_enemy(p,  4, 13, ENEMY_CRAB);
    pat_add_enemy(p, 28, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 44, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 12: castle_gate
 * Two wall pairs acting as castle gates; snail in each passage.
 * ───────────────────────────────────────────────────────────── */
static void init_pat12(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Gate 1: walls at cols 6-8 and 18-20, passage cols 9-17 */
    pat_wall(p, 9, 12,  6,  8, TILE_SOLID);
    pat_wall(p, 9, 12, 18, 20, TILE_SOLID);
    pat_set(p, 7,  3, TILE_QBLOCK);   /* open air before gate 1 */
    pat_set(p, 7, 13, TILE_QBLOCK);   /* inside passage 1 */
    /* Gate 2: walls at cols 30-32 and 42-44, passage cols 33-41 */
    pat_wall(p, 9, 12, 30, 32, TILE_SOLID);
    pat_wall(p, 9, 12, 42, 44, TILE_SOLID);
    pat_set(p, 7, 25, TILE_QBLOCK);   /* open corridor between gates */
    pat_set(p, 7, 37, TILE_QBLOCK);   /* inside passage 2 */
    pat_set(p, 9, 24, TILE_NUT);
    pat_add_enemy(p,  3, 13, ENEMY_CRAB);
    pat_add_enemy(p, 13, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 37, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 13: double_gap
 * Two pits with a floating brick island; enemies spread throughout.
 * ───────────────────────────────────────────────────────────── */
static void init_pat13(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_pit(p, 10, 13);
    pat_pit(p, 35, 38);
    /* Floating island in the middle */
    pat_hline(p, 9, 20, 28, TILE_BRICK);
    pat_set(p, 7, 22, TILE_QBLOCK);
    pat_set(p, 7, 26, TILE_QBLOCK);
    pat_set(p, 9,  5, TILE_NUT);
    pat_set(p, 9, 44, TILE_NUT);
    pat_add_enemy(p,  5, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 20, 13, ENEMY_CRAB);
    pat_add_enemy(p, 43, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 14: cascade_climb
 * Three ascending platforms form a staircase leading to a tall
 * wall.  Each platform is 2 rows higher than the last; from the
 * top platform the player can arc over the 7-tile wall, which
 * is too tall to clear from ground level alone.
 * ───────────────────────────────────────────────────────────── */
static void init_pat14(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Cascade step 1: 2 tiles above ground */
    pat_hline(p, 11,  4,  9, TILE_SOLID);
    /* Cascade step 2: 4 tiles above ground */
    pat_hline(p,  9, 13, 18, TILE_SOLID);
    /* Cascade step 3: 6 tiles above ground */
    pat_hline(p,  7, 22, 27, TILE_SOLID);
    /* Tall wall — impassable from ground, clearable from step 3 */
    pat_wall (p,  6, 12, 34, 37, TILE_SOLID);
    /* ?-blocks in open air — never directly above a platform */
    pat_set(p,  9,  2, TILE_QBLOCK);   /* before step 1, hit from ground */
    pat_set(p,  7, 11, TILE_QBLOCK);   /* between step 1 and step 2 */
    pat_set(p,  5, 20, TILE_QBLOCK);   /* between step 2 and step 3, needs step 2+ */
    /* Right-side reward */
    pat_set(p,  8, 43, TILE_QBLOCK);
    pat_set(p,  9, 46, TILE_NUT);
    pat_add_enemy(p,  2, 13, ENEMY_CRAB);
    pat_add_enemy(p, 29, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 42, 13, ENEMY_CRAB);
    pat_add_enemy(p, 48, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 15: brick_ceiling
 * Low ceiling corridor; crabs lurk before, inside, and after.
 * ───────────────────────────────────────────────────────────── */
static void init_pat15(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_hline(p, 8, 12, 38, TILE_BRICK);
    pat_set(p, 8, 11, TILE_QBLOCK);
    pat_set(p, 8, 39, TILE_QBLOCK);
    pat_set(p, 9,  5, TILE_NUT);
    pat_set(p, 9, 43, TILE_NUT);
    pat_add_enemy(p,  6, 13, ENEMY_CRAB);
    pat_add_enemy(p, 22, 13, ENEMY_CRAB);
    pat_add_enemy(p, 42, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 16: zigzag_platforms
 * Alternating high/low platforms; enemies in the valleys.
 * ───────────────────────────────────────────────────────────── */
static void init_pat16(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_hline(p, 11,  4, 11, TILE_BRICK);   /* low */
    pat_hline(p,  8, 17, 25, TILE_BRICK);   /* high */
    pat_hline(p, 11, 31, 38, TILE_BRICK);   /* low */
    pat_hline(p,  8, 43, 48, TILE_BRICK);   /* high */
    pat_set(p, 10,  7, TILE_NUT);
    pat_set(p,  7, 21, TILE_QBLOCK);
    pat_set(p, 10, 34, TILE_NUT);
    pat_set(p,  7, 45, TILE_QBLOCK);
    pat_set(p,  9, 28, TILE_NUT);
    pat_add_enemy(p, 13, 13, ENEMY_CRAB);
    pat_add_enemy(p, 28, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 41, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 17: triple_pipe
 * Three 3-tile pipes; crabs in every gap; nuts above each pipe.
 * ───────────────────────────────────────────────────────────── */
static void init_pat17(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    pat_pipe(p,  7, 3);
    pat_pipe(p, 22, 3);
    pat_pipe(p, 37, 3);
    pat_nuts_above_pipe(p,  7, 3);
    pat_nuts_above_pipe(p, 22, 3);
    pat_nuts_above_pipe(p, 37, 3);
    pat_set(p, 8, 16, TILE_QBLOCK);
    pat_set(p, 8, 30, TILE_QBLOCK);
    pat_set(p, 9,  3, TILE_NUT);
    pat_set(p, 9, 45, TILE_NUT);
    pat_add_enemy(p,  4, 13, ENEMY_CRAB);
    pat_add_enemy(p, 16, 13, ENEMY_CRAB);
    pat_add_enemy(p, 31, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 18: qblock_row
 * Six ?-blocks in a row; crabs on each side; snail in middle.
 * ───────────────────────────────────────────────────────────── */
static void init_pat18(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);
    /* Low brick shelf below the ?-blocks to guide the player */
    pat_hline(p, 11, 13, 37, TILE_BRICK);
    pat_set(p, 9, 15, TILE_QBLOCK);
    pat_set(p, 9, 18, TILE_QBLOCK);
    pat_set(p, 9, 21, TILE_QBLOCK);
    pat_set(p, 9, 25, TILE_QBLOCK);
    pat_set(p, 9, 28, TILE_QBLOCK);
    pat_set(p, 9, 31, TILE_QBLOCK);
    pat_set(p, 9,  6, TILE_NUT);
    pat_set(p, 9, 43, TILE_NUT);
    pat_add_enemy(p,  5, 13, ENEMY_CRAB);
    pat_add_enemy(p, 24, 13, ENEMY_SNAIL);
    pat_add_enemy(p, 42, 13, ENEMY_CRAB);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 19: totem_gauntlet
 * Totems (pipes) of height 1, 2, 3 repeated twice across the block.
 * Nuts above each totem; crabs patrol the open gaps.
 *
 * Layout (each | is a 2-wide pipe):
 *   [0..2] entry  |h=1|[5..10]  |h=2|[13..18]  |h=3|[21..26]
 *   |h=1|[29..34] |h=2|[37..42] |h=3|[45..49 exit]
 * ───────────────────────────────────────────────────────────── */
static void init_pat19(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);

    /* First run: h=1 at 3, h=2 at 11, h=3 at 19 */
    pat_pipe(p,  3, 1);
    pat_pipe(p, 11, 2);
    pat_pipe(p, 19, 3);
    pat_nuts_above_pipe(p,  3, 1);
    pat_nuts_above_pipe(p, 11, 2);
    pat_nuts_above_pipe(p, 19, 3);

    /* Second run: h=1 at 27, h=2 at 35, h=3 at 43 */
    pat_pipe(p, 27, 1);
    pat_pipe(p, 35, 2);
    pat_pipe(p, 43, 3);
    pat_nuts_above_pipe(p, 27, 1);
    pat_nuts_above_pipe(p, 35, 2);
    pat_nuts_above_pipe(p, 43, 3);

    /* Crabs in the longer gaps */
    pat_add_enemy(p,  7, 13, ENEMY_CRAB);
    pat_add_enemy(p, 24, 13, ENEMY_CRAB);
    pat_add_enemy(p, 32, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Pattern 20: tall_totem_gauntlet
 * Like totem_gauntlet but taller: heights 2, 3, 4 repeated twice.
 * The h=4 pipe tops are at row 9 — still clearable, but tight.
 * ───────────────────────────────────────────────────────────── */
static void init_pat20(Pattern *p) {
    memset(p, 0, sizeof(*p));
    pat_ground(p);

    /* First run: h=2 at 3, h=3 at 11, h=4 at 19 */
    pat_pipe(p,  3, 2);
    pat_pipe(p, 11, 3);
    pat_pipe(p, 19, 4);
    pat_nuts_above_pipe(p,  3, 2);
    pat_nuts_above_pipe(p, 11, 3);
    pat_nuts_above_pipe(p, 19, 4);

    /* Second run: h=2 at 27, h=3 at 35, h=4 at 43 */
    pat_pipe(p, 27, 2);
    pat_pipe(p, 35, 3);
    pat_pipe(p, 43, 4);
    pat_nuts_above_pipe(p, 27, 2);
    pat_nuts_above_pipe(p, 35, 3);
    pat_nuts_above_pipe(p, 43, 4);

    /* Crabs in the longer gaps */
    pat_add_enemy(p,  7, 13, ENEMY_CRAB);
    pat_add_enemy(p, 24, 13, ENEMY_CRAB);
    pat_add_enemy(p, 32, 13, ENEMY_SNAIL);
}

/* ─────────────────────────────────────────────────────────────
 * Public API
 * ───────────────────────────────────────────────────────────── */

void mapgen_init_patterns(void) {
    init_pat0 (&g_patterns[ 0]);
    init_pat1 (&g_patterns[ 1]);
    init_pat2 (&g_patterns[ 2]);
    init_pat3 (&g_patterns[ 3]);
    init_pat4 (&g_patterns[ 4]);
    init_pat5 (&g_patterns[ 5]);
    init_pat6 (&g_patterns[ 6]);
    init_pat7 (&g_patterns[ 7]);
    init_pat8 (&g_patterns[ 8]);
    init_pat9 (&g_patterns[ 9]);
    init_pat10(&g_patterns[10]);
    init_pat11(&g_patterns[11]);
    init_pat12(&g_patterns[12]);
    init_pat13(&g_patterns[13]);
    init_pat14(&g_patterns[14]);
    init_pat15(&g_patterns[15]);
    init_pat16(&g_patterns[16]);
    init_pat17(&g_patterns[17]);
    init_pat18(&g_patterns[18]);
    init_pat19(&g_patterns[19]);
    init_pat20(&g_patterns[20]);
}

void mapgen_write_block(uint8_t *dst, int buf_block, const Pattern *pat) {
    for (int row = 0; row < LEVEL_MAX_H; row++) {
        memcpy(
            dst + row * MAP_BUF_W + buf_block * BLOCK_W,
            pat->tiles + row * BLOCK_W,
            BLOCK_W
        );
    }
}

int mapgen_pick_next(int excluded) {
    int id;
    do { id = rng_next(NUM_PATTERNS); } while (id == excluded);
    return id;
}

void mapgen_init_buffer(uint8_t *dst, int out_ids[3]) {
    out_ids[0] = mapgen_pick_next(-1);
    out_ids[1] = mapgen_pick_next(out_ids[0]);
    out_ids[2] = mapgen_pick_next(out_ids[1]);
    mapgen_write_block(dst, 0, &g_patterns[out_ids[0]]);
    mapgen_write_block(dst, 1, &g_patterns[out_ids[1]]);
    mapgen_write_block(dst, 2, &g_patterns[out_ids[2]]);
}
