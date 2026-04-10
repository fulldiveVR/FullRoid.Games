#include "enemy.h"
#include <stddef.h>

#define SQUISH_TIME  300   /* ms crab squish is visible */

void enemy_spawn(Enemy *e, EnemyType type, int wx, int wy) {
    e->type         = type;
    e->status       = ENEMY_ALIVE;
    e->x_fp         = INT_TO_FP(wx);
    e->y_fp         = INT_TO_FP(wy);
    e->vx_fp        = -ENEMY_SPEED;
    e->vy_fp        = 0;
    e->on_ground    = 0;
    e->squish_timer = 0;
}

static int tile_solid_at(const Level *lvl, int wx, int wy) {
    int tx = wx / TILE_PX;
    int ty = wy / TILE_PX;
    return tile_is_solid(level_tile(lvl, tx, ty));
}

void enemy_update(Enemy *e, const Level *lvl, int delta_ms) {
    if (e->status == ENEMY_DEAD) return;

    /* Squish timer */
    if (e->status == ENEMY_SQUISHED) {
        e->squish_timer -= delta_ms;
        if (e->squish_timer <= 0)
            e->status = ENEMY_DEAD;
        return;
    }

    /* ── Gravity ── */
    e->vy_fp += FP_MUL(GRAVITY, INT_TO_FP(delta_ms)) / 1000;
    if (e->vy_fp > INT_TO_FP(400)) e->vy_fp = INT_TO_FP(400);

    /* ── Move X ── */
    e->x_fp += FP_MUL(e->vx_fp, INT_TO_FP(delta_ms)) / 1000;
    {
        int wx = FP_TO_INT(e->x_fp);
        int wy = FP_TO_INT(e->y_fp);
        if (e->vx_fp < 0 && tile_solid_at(lvl, wx, wy + ENEMY_H / 2)) {
            e->x_fp  = INT_TO_FP((wx / TILE_PX + 1) * TILE_PX);
            e->vx_fp = -e->vx_fp;
        } else if (e->vx_fp > 0 &&
                   tile_solid_at(lvl, wx + ENEMY_W - 1, wy + ENEMY_H / 2)) {
            e->x_fp  = INT_TO_FP(((wx + ENEMY_W - 1) / TILE_PX) * TILE_PX - ENEMY_W);
            e->vx_fp = -e->vx_fp;
        }
    }

    /* ── Move Y ── */
    e->y_fp += FP_MUL(e->vy_fp, INT_TO_FP(delta_ms)) / 1000;
    e->on_ground = 0;
    {
        int wx = FP_TO_INT(e->x_fp);
        int wy = FP_TO_INT(e->y_fp);
        if (e->vy_fp >= 0) {
            int bot = wy + ENEMY_H;
            if (tile_solid_at(lvl, wx + 4, bot) ||
                tile_solid_at(lvl, wx + ENEMY_W - 4, bot)) {
                e->y_fp      = INT_TO_FP((bot / TILE_PX) * TILE_PX - ENEMY_H);
                e->vy_fp     = 0;
                e->on_ground = 1;
            }
        }
        if (e->vy_fp < 0) {
            if (tile_solid_at(lvl, wx + 4, wy) ||
                tile_solid_at(lvl, wx + ENEMY_W - 4, wy)) {
                e->y_fp  = INT_TO_FP((wy / TILE_PX + 1) * TILE_PX);
                e->vy_fp = 0;
            }
        }
    }

    /* Reverse at ledge edge — both crabs and snails don't walk off */
    if (e->on_ground) {
        int wx   = FP_TO_INT(e->x_fp);
        int wy   = FP_TO_INT(e->y_fp);
        int foot = wy + ENEMY_H;
        if (e->vx_fp < 0 && !tile_solid_at(lvl, wx - 1, foot))
            e->vx_fp = ENEMY_SPEED;
        if (e->vx_fp > 0 && !tile_solid_at(lvl, wx + ENEMY_W, foot))
            e->vx_fp = -ENEMY_SPEED;
    }

    /* Fall out of world */
    if (FP_TO_INT(e->y_fp) > SCREEN_H + 64)
        e->status = ENEMY_DEAD;
}
