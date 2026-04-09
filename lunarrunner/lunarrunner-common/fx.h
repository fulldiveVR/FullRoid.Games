#pragma once

#define MAX_PARTICLES 16

typedef enum {
    FX_NONE,
    FX_COLLECT_FLASH,
    FX_SHIELD_BREAK,
    FX_METEOR_WARN,
    FX_TURBO_LINES,
    FX_DEATH
} FxType;

typedef struct {
    int x, y;
    int vx, vy;
    int life;
    int color_idx;
} Particle;

typedef struct {
    FxType    active_fx;
    int       fx_timer;
    Particle  particles[MAX_PARTICLES];
    int       screen_shake_x;
    int       screen_shake_y;
} FxState;

void fx_init(FxState *fx);
void fx_trigger(FxState *fx, FxType type, int x, int y);
void fx_tick(FxState *fx, int delta_ms);
