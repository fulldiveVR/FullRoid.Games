#include "fx.h"
#include "rng.h"

void fx_init(FxState *fx) {
    fx->active_fx      = FX_NONE;
    fx->fx_timer       = 0;
    fx->screen_shake_x = 0;
    fx->screen_shake_y = 0;
    for (int i = 0; i < MAX_PARTICLES; i++)
        fx->particles[i].life = 0;
}

static void spawn_particles(FxState *fx, int x, int y, int count, int color) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        Particle *p = &fx->particles[i];
        if (p->life > 0) continue;
        p->x = x;
        p->y = y;
        p->vx = rng_next(200) - 100;
        p->vy = rng_next(200) - 150;
        p->life = 300 + rng_next(200);
        p->color_idx = color;
        spawned++;
    }
}

void fx_trigger(FxState *fx, FxType type, int x, int y) {
    fx->active_fx = type;
    switch (type) {
    case FX_COLLECT_FLASH:
        fx->fx_timer = 100;
        spawn_particles(fx, x, y, 4, 8); /* yellow */
        break;
    case FX_SHIELD_BREAK:
        fx->fx_timer = 200;
        spawn_particles(fx, x, y, 8, 7); /* blue */
        break;
    case FX_METEOR_WARN:
        fx->fx_timer = 1000;
        break;
    case FX_TURBO_LINES:
        fx->fx_timer = 100;
        break;
    case FX_DEATH:
        fx->fx_timer = 400;
        fx->screen_shake_x = 3;
        fx->screen_shake_y = 2;
        break;
    default:
        fx->fx_timer = 0;
        break;
    }
}

void fx_tick(FxState *fx, int delta_ms) {
    /* Active effect timer */
    if (fx->fx_timer > 0) {
        fx->fx_timer -= delta_ms;
        if (fx->fx_timer <= 0) {
            fx->fx_timer       = 0;
            fx->active_fx      = FX_NONE;
            fx->screen_shake_x = 0;
            fx->screen_shake_y = 0;
        }
    }

    /* Update particles */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->particles[i];
        if (p->life <= 0) continue;
        p->life -= delta_ms;
        p->x += p->vx * delta_ms / 1000;
        p->y += p->vy * delta_ms / 1000;
        p->vy += 400 * delta_ms / 1000; /* particle gravity */
    }
}
