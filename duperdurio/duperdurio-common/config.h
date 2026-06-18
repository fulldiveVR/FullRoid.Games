#pragma once

/* --- Fixed-point arithmetic (shift 8) --- */
#define FP_SHIFT         8
#define FP_ONE           (1 << FP_SHIFT)
#define FP_MUL(a, b)     (((a) * (b)) >> FP_SHIFT)
#define FP_DIV(a, b)     (((a) << FP_SHIFT) / (b))
#define FP_TO_INT(a)     ((a) >> FP_SHIFT)
#define INT_TO_FP(a)     ((a) << FP_SHIFT)

/* Integrate an fp-per-second value over delta_ms (x is already fixed-point).
   Uses int64 internally: FP_MUL(x, INT_TO_FP(delta_ms)) overflows int32 once x
   is HD-scaled (e.g. GRAVITY*INT_TO_FP(16) > 2^31), which silently flips the
   sign and sends entities flying upward. */
#define FP_STEP(x, dt)   ((int32_t)((int64_t)(x) * (dt) / 1000))

/* --- Screen (3DS only target) --- */
#define SCREEN_W         400
#define SCREEN_H         240
#define BOT_SCREEN_W     320
#define BOT_SCREEN_H     240

/* --- Tile grid --- */
/* HD: 24px tiles. The level is LEVEL_MAX_H*24 = 360px tall — taller than the
   240px screen — so the camera scrolls vertically (Level.cam_y). The 21 map
   patterns keep their 15-row layout; world-space physics constants below are
   scaled x1.5 vs the old 16px grid so jump arcs / gaps stay the same in tiles. */
#define TILE_SIZE        24   /* logical game tile in px */
#define TILE_SCALE       1    /* render scale */
#define TILE_PX          (TILE_SIZE * TILE_SCALE)   /* 24 — drawn tile size */

/* Ground level: y-coord of the top of the ground row in screen pixels */
#define GROUND_Y         (SCREEN_H - TILE_PX)       /* = 208 */

/* --- Durio sprite --- */
#define DURIO_W          TILE_PX          /* 16 px wide */
#define DURIO_H          TILE_PX          /* 16 px tall */

/* Hitbox insets (shrink drawn rect for collision) — scaled for 24px tiles */
#define DURIO_HIT_INSET_X   3
#define DURIO_HIT_INSET_TOP 3
#define DURIO_HIT_INSET_BOT 2

/* --- Enemy sprite --- */
#define ENEMY_W          TILE_PX          /* 32 px */
#define ENEMY_H          TILE_PX          /* 32 px */
#define ENEMY_H_SQUISH   (TILE_PX / 2)   /* 16 px (squished Crab) */

/* --- Physics (fixed-point units per second) --- scaled x1.5 for 24px tiles */
#define GRAVITY          INT_TO_FP(2100)  /* downward accel per second */
#define JUMP_VELOCITY    INT_TO_FP(-780)  /* initial jump vel (upward) */
#define JUMP_HOLD_MIN    150              /* ms: min time A held for full jump */
#define WALK_SPEED       INT_TO_FP(150)
#define RUN_SPEED        INT_TO_FP(270)
#define ENEMY_SPEED      INT_TO_FP(90)

/* --- Game limits --- */
#define MAX_ENEMIES      16
#define MAX_NUTS        32
#define LEVEL_MAX_H      15    /* tiles tall (15 rows) */

/* --- Infinite map --- */
#define BLOCK_W          50              /* tiles per block = 2 × 400px screens */
#define MAP_BUF_BLOCKS   3               /* prev + current + next */
#define MAP_BUF_W        (MAP_BUF_BLOCKS * BLOCK_W)   /* 150 tiles buffer width */

/* Back-scroll: player cannot go more than half a screen behind their furthest point */
#define BACKSCROLL_PX    (SCREEN_W / 2)  /* 200 px */

/* --- Tile types --- */
#define TILE_AIR         0
#define TILE_GROUND      1
#define TILE_BRICK       2
#define TILE_QBLOCK      3   /* ? block (active) */
#define TILE_QBLOCK_USED 4   /* ? block (empty) */
#define TILE_PIPE_TOP    5
#define TILE_PIPE_BODY   6
#define TILE_NUT         7   /* nut embedded in level */
#define TILE_SOLID       10  /* generic solid (castle wall, etc.) */
#define TILE_SHELL       11  /* snail shell — solid obstacle, placed on stomp */

/* --- Scoring --- */
#define SCORE_NUT       500   /* per nut collected */
#define SCORE_CRAB     500   /* per enemy stomped */
#define SCORE_SNAIL      500
#define SCORE_TILE       10    /* per new tile reached (max distance) */

