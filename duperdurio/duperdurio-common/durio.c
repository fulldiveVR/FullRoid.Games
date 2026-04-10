#include "durio.h"
#include <stddef.h>

#define DURIO_START_X    48
#define DURIO_START_ROW  12   /* tile row (ground row - 1) */

#define WALK_FRAME_MS    120  /* ms per walk animation frame */
#define DEATH_FLY_VY    INT_TO_FP(-600)

void durio_init(Durio *m) {
    m->x_fp         = INT_TO_FP(DURIO_START_X);
    m->y_fp         = INT_TO_FP(DURIO_START_ROW * TILE_PX - DURIO_H);
    m->vx_fp        = 0;
    m->vy_fp        = 0;
    m->state        = DURIO_SMALL;
    m->dir          = DURIO_FACE_RIGHT;
    m->on_ground        = 0;
    m->coyote_ms        = 0;
    m->jump_buf_ms      = 0;
    m->pending_coin_tx  = -1;
    m->pending_coin_ty  = -1;
    m->jump_held        = 0;
    m->jump_held_ms = 0;
    m->dead_timer   = 0;
    m->walk_frame   = 0;
    m->walk_timer   = 0;
    m->nuts         = 0;
    m->score        = 0;
    m->lives        = 3;
    m->max_tile_x   = 0;
}

/* ─────────────────────────────────────────────────────────────
 * Tile collision helpers
 * ───────────────────────────────────────────────────────────── */

/* Test if a world-pixel rect overlaps any solid tile.
   Returns 1 if collision found, sets *tx / *ty to the tile. */
static int collide_tiles_rect(const Level *lvl,
                               int wx, int wy, int w, int h,
                               int *out_tx, int *out_ty) {
    int tx0 = wx / TILE_PX;
    int ty0 = wy / TILE_PX;
    int tx1 = (wx + w - 1) / TILE_PX;
    int ty1 = (wy + h - 1) / TILE_PX;
    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tile_is_solid(level_tile(lvl, tx, ty))) {
                if (out_tx) *out_tx = tx;
                if (out_ty) *out_ty = ty;
                return 1;
            }
        }
    }
    return 0;
}

void durio_hit_block(Durio *m, Level *lvl, int tx, int ty) {
    uint8_t t = level_tile(lvl, tx, ty);
    if (tile_is_qblock(t)) {
        /* Mark block used; signal game.c to spawn the visual coin */
        level_set_tile(lvl, tx, ty, TILE_QBLOCK_USED);
        m->pending_coin_tx = tx;
        m->pending_coin_ty = ty;
        m->nuts++;
        m->score += SCORE_NUT;
    } else if (tile_is_brick(t)) {
        /* Bounce off brick (no break), handled by physics rebound */
    }
}

/* ─────────────────────────────────────────────────────────────
 * Main update
 * ───────────────────────────────────────────────────────────── */

#define COYOTE_TIME_MS  100   /* ms after leaving ground where jump still works */
#define JUMP_BUF_MS     120   /* ms before landing where a queued jump fires */

void durio_update(Durio *m, Level *lvl,
                  int inp_left, int inp_right, int inp_run,
                  int inp_jump_down, int inp_jump_held,
                  int delta_ms) {

    /* Death animation: brief pause, then fly up and fall */
    if (m->state == DURIO_DEAD) {
        m->dead_timer += delta_ms;
        if (m->dead_timer >= 300) {
            if (m->dead_timer - delta_ms < 300)
                m->vy_fp = DEATH_FLY_VY;   /* set once on transition */
            m->vy_fp += FP_MUL(GRAVITY, INT_TO_FP(delta_ms)) / 1000;
            m->y_fp  += FP_MUL(m->vy_fp,  INT_TO_FP(delta_ms)) / 1000;
        }
        return;
    }

    int h = durio_height(m);

    /* ── Coyote time: refresh on ground, drain in air ── */
    if (m->on_ground) {
        m->coyote_ms = COYOTE_TIME_MS;
    } else {
        m->coyote_ms -= delta_ms;
        if (m->coyote_ms < 0) m->coyote_ms = 0;
    }

    /* ── Jump buffer: remember a press for up to JUMP_BUF_MS ── */
    if (inp_jump_down) {
        m->jump_buf_ms = JUMP_BUF_MS;
    } else if (m->jump_buf_ms > 0) {
        m->jump_buf_ms -= delta_ms;
        if (m->jump_buf_ms < 0) m->jump_buf_ms = 0;
    }

    /* ── Horizontal acceleration ── */
    int32_t target_spd = inp_run ? RUN_SPEED : WALK_SPEED;
    if (inp_left && !inp_right) {
        m->dir = DURIO_FACE_LEFT;
        if (m->vx_fp > -target_spd) {
            m->vx_fp -= INT_TO_FP(12);
            if (m->vx_fp < -target_spd) m->vx_fp = -target_spd;
        }
    } else if (inp_right && !inp_left) {
        m->dir = DURIO_FACE_RIGHT;
        if (m->vx_fp < target_spd) {
            m->vx_fp += INT_TO_FP(12);
            if (m->vx_fp > target_spd) m->vx_fp = target_spd;
        }
    } else {
        /* Decelerate */
        if (m->vx_fp > INT_TO_FP(8))       m->vx_fp -= INT_TO_FP(8);
        else if (m->vx_fp < -INT_TO_FP(8)) m->vx_fp += INT_TO_FP(8);
        else                                m->vx_fp = 0;
    }

    /* ── Jump: fire if buffered press + coyote window available ── */
    if ((inp_jump_down || m->jump_buf_ms > 0) && m->coyote_ms > 0) {
        m->vy_fp        = JUMP_VELOCITY;
        m->coyote_ms    = 0;
        m->jump_buf_ms  = 0;
        m->jump_held    = 1;
        m->jump_held_ms = 0;
    }

    /* ── Variable jump height: extra boost while A/B held ── */
    if (m->jump_held) {
        if (inp_jump_held && m->jump_held_ms < JUMP_HOLD_MIN) {
            m->jump_held_ms += delta_ms;
            m->vy_fp -= INT_TO_FP(6);
        } else {
            m->jump_held = 0;
        }
    }

    /* ── Gravity ── */
    m->vy_fp += FP_MUL(GRAVITY, INT_TO_FP(delta_ms)) / 1000;
    if (m->vy_fp > INT_TO_FP(400)) m->vy_fp = INT_TO_FP(400);

    /* ── Move X, resolve X collisions (hitbox insets) ── */
    m->x_fp += FP_MUL(m->vx_fp, INT_TO_FP(delta_ms)) / 1000;
    {
        int wx = FP_TO_INT(m->x_fp);
        int wy = FP_TO_INT(m->y_fp);
        int hit_tx, hit_ty;
        if (collide_tiles_rect(lvl,
                               wx + DURIO_HIT_INSET_X,
                               wy + DURIO_HIT_INSET_TOP,
                               DURIO_W - DURIO_HIT_INSET_X * 2,
                               h - DURIO_HIT_INSET_TOP - DURIO_HIT_INSET_BOT,
                               &hit_tx, &hit_ty)) {
            int tile_px_x = hit_tx * TILE_PX;
            if (m->vx_fp > 0)
                m->x_fp = INT_TO_FP(tile_px_x - DURIO_W);
            else
                m->x_fp = INT_TO_FP(tile_px_x + TILE_PX);
            m->vx_fp = 0;
        }
        if (m->x_fp < 0) { m->x_fp = 0; m->vx_fp = 0; }
    }

    /* ── Move Y, resolve Y collisions ── */
    m->y_fp += FP_MUL(m->vy_fp, INT_TO_FP(delta_ms)) / 1000;
    m->on_ground = 0;
    {
        int wx  = FP_TO_INT(m->x_fp);
        int wy  = FP_TO_INT(m->y_fp);
        int tx0 = (wx + DURIO_HIT_INSET_X) / TILE_PX;
        int tx1 = (wx + DURIO_W - DURIO_HIT_INSET_X - 1) / TILE_PX;

        if (m->vy_fp >= 0) {
            /* Falling / stationary: probe tile at sprite feet */
            int ty = (wy + h) / TILE_PX;
            for (int tx = tx0; tx <= tx1; tx++) {
                if (tile_is_solid(level_tile(lvl, tx, ty))) {
                    m->y_fp      = INT_TO_FP(ty * TILE_PX - h);
                    m->vy_fp     = 0;
                    m->on_ground = 1;
                    m->jump_held = 0;
                    break;
                }
            }
        } else {
            /* Rising: probe tile at sprite head (with top inset) */
            int ty = (wy + DURIO_HIT_INSET_TOP) / TILE_PX;
            for (int tx = tx0; tx <= tx1; tx++) {
                if (tile_is_solid(level_tile(lvl, tx, ty))) {
                    m->y_fp  = INT_TO_FP((ty + 1) * TILE_PX
                                         - DURIO_HIT_INSET_TOP);
                    durio_hit_block(m, lvl, tx, ty);
                    m->vy_fp = 0;
                    break;
                }
            }
        }
    }

    /* Fall out of world = die */
    if (FP_TO_INT(m->y_fp) > SCREEN_H + 64)
        durio_kill(m);

    /* ── Walk animation ── */
    if (m->on_ground && (inp_left || inp_right)) {
        m->walk_timer += delta_ms;
        while (m->walk_timer >= WALK_FRAME_MS) {
            m->walk_timer -= WALK_FRAME_MS;
            m->walk_frame = (m->walk_frame + 1) % 3;
        }
    } else if (!inp_left && !inp_right) {
        m->walk_frame = 0;
        m->walk_timer = 0;
    }
}

void durio_respawn(Durio *m, int32_t spawn_x_fp) {
    /* Place Durio just above the top of the screen so it falls in */
    m->x_fp         = spawn_x_fp;
    m->y_fp         = INT_TO_FP(-DURIO_H);
    m->vx_fp        = 0;
    m->vy_fp        = 0;
    m->state        = DURIO_SMALL;
    m->dir          = DURIO_FACE_RIGHT;
    m->on_ground    = 0;
    m->coyote_ms    = 0;
    m->jump_buf_ms  = 0;
    m->pending_coin_tx = -1;
    m->pending_coin_ty = -1;
    m->jump_held    = 0;
    m->jump_held_ms = 0;
    m->dead_timer   = 0;
    m->walk_frame   = 0;
    m->walk_timer   = 0;
    m->max_tile_x   = 0;
    /* score, coins, lives preserved */
}

void durio_kill(Durio *m) {
    if (m->state == DURIO_DEAD) return;
    m->state     = DURIO_DEAD;
    m->dead_timer = 0;
    m->vy_fp     = 0;
    m->vx_fp     = 0;
    m->lives--;
}
