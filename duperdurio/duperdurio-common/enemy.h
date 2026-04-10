#pragma once
#include "config.h"
#include "level.h"
#include "physics.h"
#include <stdint.h>

typedef enum {
    ENEMY_NONE  = 0,
    ENEMY_CRAB  = 1,
    ENEMY_SNAIL = 2
} EnemyType;

typedef enum {
    ENEMY_ALIVE,
    ENEMY_SQUISHED,   /* Crab: brief squish animation then removed */
    ENEMY_DEAD        /* Remove next frame */
} EnemyStatus;

typedef struct {
    EnemyType   type;
    EnemyStatus status;

    int32_t x_fp;    /* left edge, world pixels */
    int32_t y_fp;    /* top edge, world pixels */
    int32_t vx_fp;
    int32_t vy_fp;

    int on_ground;
    int squish_timer;   /* ms remaining for squish anim */
} Enemy;

/* Spawn an enemy of given type at world pixel (wx, wy) */
void enemy_spawn(Enemy *e, EnemyType type, int wx, int wy);

/* Update one enemy: physics + tile collision + reversal */
void enemy_update(Enemy *e, const Level *lvl, int delta_ms);

/* Enemy hitbox in world pixels */
static inline Rect enemy_hitbox(const Enemy *e) {
    Rect r = {
        FP_TO_INT(e->x_fp),
        FP_TO_INT(e->y_fp),
        ENEMY_W,
        (e->status == ENEMY_SQUISHED) ? ENEMY_H_SQUISH : ENEMY_H
    };
    return r;
}
