#pragma once
#include "config.h"

typedef enum {
    BONUS_NONE,
    BONUS_SOLAR,
    BONUS_TURBO,
    BONUS_MAGNET
} BonusType;

typedef struct {
    int       bar;
    BonusType stored;
    int       hold_timer;
    int       solar_timer;
    int       turbo_timer;
    int       magnet_timer;
} BonusState;

void bonus_init(BonusState *b);
void bonus_tick(BonusState *b, int delta_ms);
void bonus_activate(BonusState *b);
void bonus_add_bar(BonusState *b, int amount);

/* Returns current score multiplier (1 or 2) */
int  bonus_score_multiplier(const BonusState *b);
int  bonus_is_turbo(const BonusState *b);
int  bonus_is_magnet(const BonusState *b);
