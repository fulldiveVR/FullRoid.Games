#pragma once
#include "board.h"
#include "anim.h"

typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_WIN,
    STATE_GAME_OVER
} GameState;

typedef struct {
    Board     board;
    AnimState anim;
    GameState state;
    int       best_score;
    int       continued;
    int       spawn_pending;  /* 1 = spawn after slide animation finishes */
} Game;

void game_init(Game *g);
void game_start(Game *g);
void game_move(Game *g, SlideDir dir);

/* Call every frame — advances animations, spawns tile when slide finishes */
void game_tick(Game *g, int delta_ms);

/* Returns 1 if animations are still playing (input should be blocked) */
int  game_anim_busy(const Game *g);
