#pragma once
#include "snake.h"
#include "food.h"

typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_WIN,
    STATE_LOSE
} GameState;

typedef struct {
    Snake     snake;
    Food      food;
    GameState state;
    int       autopilot;
} Game;

void game_init(Game *g);
void game_start(Game *g);
void game_update(Game *g);

/* Возвращает длительность тика в мс для текущей длины змейки */
int  game_tick_ms(const Game *g);
