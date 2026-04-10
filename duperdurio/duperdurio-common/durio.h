#pragma once
#include "config.h"
#include "level.h"
#include "physics.h"
#include <stdint.h>

typedef enum {
    DURIO_SMALL,
    DURIO_DEAD
} DurioState;

typedef enum {
    DURIO_FACE_RIGHT = 0,
    DURIO_FACE_LEFT  = 1
} DurioDir;

typedef struct {
    /* Position in fixed-point world pixels */
    int32_t x_fp;   /* left edge */
    int32_t y_fp;   /* top edge */

    /* Velocity in fixed-point pixels per second */
    int32_t vx_fp;
    int32_t vy_fp;

    DurioState state;
    DurioDir   dir;

    int on_ground;        /* 1 = feet touching solid tile */
    int coyote_ms;        /* coyote time: jump allowed N ms after walking off edge */
    int jump_buf_ms;      /* jump buffer: queued jump press, applied on next landing */
    int pending_coin_tx;  /* >=0: ?-block hit this frame, game.c must spawn a coin */
    int pending_coin_ty;
    int jump_held;       /* 1 = A/B held for variable height boost */
    int jump_held_ms;    /* ms A/B held since jump started */
    int dead_timer;      /* ms since death started (for animation) */

    /* Animation */
    int walk_frame;      /* 0-2 walk cycle tile index */
    int walk_timer;      /* ms accumulator for frame advance */

    /* Score / stats */
    int nuts;
    int score;
    int lives;
    int max_tile_x;   /* furthest tile column reached (for distance score) */
} Durio;

/* Initialise Durio at starting position */
void durio_init(Durio *m);

/* Update physics, movement, tile collisions.
   inp_left/right/run: held this frame.
   inp_jump_down: jump pressed this frame.
   inp_jump_held: jump button currently held. */
void durio_update(Durio *m, Level *lvl,
                  int inp_left, int inp_right, int inp_run,
                  int inp_jump_down, int inp_jump_held,
                  int delta_ms);

/* Called when Durio stomps a block from below (tx, ty = tile coords) */
void durio_hit_block(Durio *m, Level *lvl, int tx, int ty);

/* Kill Durio (set DEAD, start death animation) */
void durio_kill(Durio *m);

/* Respawn after losing a life: reset position/physics, preserve score/lives.
   spawn_x_fp = world-pixel X (fixed-point) where Durio will fall in from top. */
void durio_respawn(Durio *m, int32_t spawn_x_fp);

/* Screen X of Durio's left edge (world_x - cam_x) */
static inline int durio_screen_x(const Durio *m, int cam_x) {
    return FP_TO_INT(m->x_fp) - cam_x;
}
static inline int durio_screen_y(const Durio *m) {
    return FP_TO_INT(m->y_fp);
}
static inline int durio_height(const Durio *m) {
    (void)m;
    return DURIO_H;
}

/* Hitbox in world pixel coords */
static inline Rect durio_hitbox(const Durio *m) {
    int h = durio_height(m);
    Rect r = {
        FP_TO_INT(m->x_fp) + DURIO_HIT_INSET_X,
        FP_TO_INT(m->y_fp) + DURIO_HIT_INSET_TOP,
        DURIO_W - DURIO_HIT_INSET_X * 2,
        h - DURIO_HIT_INSET_TOP - DURIO_HIT_INSET_BOT
    };
    return r;
}
