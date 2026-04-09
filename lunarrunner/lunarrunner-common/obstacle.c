#include "obstacle.h"

void obstacle_init_all(Obstacle obs[], MeteorRain *meteor) {
    for (int i = 0; i < MAX_OBSTACLES; i++)
        obs[i].active = 0;
    meteor_init(meteor);
}

void obstacle_move_all(Obstacle obs[], int dx_px) {
    int dx_fp = INT_TO_FP(dx_px);
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obs[i].active) continue;
        obs[i].x_fp -= dx_fp;
        if (FP_TO_INT(obs[i].x_fp) + obs[i].w < 0)
            obs[i].active = 0;
    }
}

Obstacle *obstacle_spawn(Obstacle obs[], ObstacleType type, int variant) {
    Obstacle *o = 0;
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obs[i].active) { o = &obs[i]; break; }
    }
    if (!o) return 0;

    o->type    = type;
    o->x_fp    = INT_TO_FP(SCREEN_W + 16);
    o->active  = 1;
    o->variant = variant;

    /* Hitbox sizes tuned so jump (0.7s) comfortably clears at any speed.
       At 80 px/s the rover covers 56px in a jump; rover_w=24,
       so obstacle_w must be << 32 to leave reaction margin. */
    switch (type) {
    case OBS_CRATER:
        o->w = 14; o->h = 6;
        o->y = GROUND_Y - 6;
        break;
    case OBS_BOULDER:
        o->w = 12; o->h = 18;
        o->y = GROUND_Y - 18;
        break;
    case OBS_ANTENNA:
        o->w = 24; o->h = 5;
        o->y = GROUND_Y - ROVER_H - 2;
        break;
    case OBS_ALIEN_FLOWER:
        if (variant == 0) {
            o->w = 10; o->h = 14;
            o->y = GROUND_Y - 16;
        } else {
            o->w = 10; o->h = 8;
            o->y = GROUND_Y - 8;
        }
        break;
    }
    return o;
}

void meteor_init(MeteorRain *m) {
    m->active        = 0;
    m->warning_timer = 0;
    m->rain_timer    = 0;
}

void meteor_tick(MeteorRain *m, int delta_ms) {
    if (!m->active) return;

    if (m->warning_timer > 0) {
        m->warning_timer -= delta_ms;
        if (m->warning_timer <= 0) {
            m->warning_timer = 0;
            m->rain_timer = 2500;
        }
    } else if (m->rain_timer > 0) {
        m->rain_timer -= delta_ms;
        if (m->rain_timer <= 0) {
            m->rain_timer = 0;
            m->active = 0;
        }
    }
}
