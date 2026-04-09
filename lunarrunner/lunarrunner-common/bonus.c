#include "bonus.h"
#include "rng.h"

void bonus_init(BonusState *b) {
    b->bar          = 0;
    b->stored       = BONUS_NONE;
    b->hold_timer   = 0;
    b->solar_timer  = 0;
    b->turbo_timer  = 0;
    b->magnet_timer = 0;
}

void bonus_tick(BonusState *b, int delta_ms) {
    if (b->solar_timer > 0) {
        b->solar_timer -= delta_ms;
        if (b->solar_timer < 0) b->solar_timer = 0;
    }
    if (b->turbo_timer > 0) {
        b->turbo_timer -= delta_ms;
        if (b->turbo_timer < 0) b->turbo_timer = 0;
    }
    if (b->magnet_timer > 0) {
        b->magnet_timer -= delta_ms;
        if (b->magnet_timer < 0) b->magnet_timer = 0;
    }
    if (b->stored != BONUS_NONE) {
        b->hold_timer -= delta_ms;
        if (b->hold_timer <= 0) {
            b->stored     = BONUS_NONE;
            b->hold_timer = 0;
        }
    }
}

void bonus_add_bar(BonusState *b, int amount) {
    b->bar += amount;
    if (b->bar >= BONUS_BAR_MAX && b->stored == BONUS_NONE) {
        b->bar -= BONUS_BAR_MAX;
        /* Random bonus from pool: solar, turbo, magnet */
        int r = rng_next(3);
        if (r == 0)      b->stored = BONUS_SOLAR;
        else if (r == 1) b->stored = BONUS_TURBO;
        else             b->stored = BONUS_MAGNET;
        b->hold_timer = BONUS_HOLD_TIME;
    }
}

void bonus_activate(BonusState *b) {
    if (b->stored == BONUS_NONE) return;
    switch (b->stored) {
    case BONUS_SOLAR:  b->solar_timer  = BONUS_SOLAR_DUR;  break;
    case BONUS_TURBO:  b->turbo_timer  = BONUS_TURBO_DUR;  break;
    case BONUS_MAGNET: b->magnet_timer = BONUS_MAGNET_DUR; break;
    default: break;
    }
    b->stored     = BONUS_NONE;
    b->hold_timer = 0;
}

int bonus_score_multiplier(const BonusState *b) {
    return b->solar_timer > 0 ? 2 : 1;
}

int bonus_is_turbo(const BonusState *b) {
    return b->turbo_timer > 0;
}

int bonus_is_magnet(const BonusState *b) {
    return b->magnet_timer > 0;
}
