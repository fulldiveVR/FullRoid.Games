#pragma once
#include "config.h"
#include "runner.h"
#include "world.h"
#include "obstacle.h"
#include "collectible.h"
#include "bonus.h"
#include "fx.h"

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

typedef struct {
    int jump;
    int duck;
    int duck_released;
    int bonus;
    int pause;
    int confirm;
    int back;
    int lang_next;
    int lang_prev;
} InputState;

typedef struct {
    GameState    state;
    Rover        rover;
    World        world;
    BonusState   bonus;
    MeteorRain   meteor;
    FxState      fx;
    Obstacle     obstacles[MAX_OBSTACLES];
    Collectible  collectibles[MAX_COLLECTIBLES];
    int          score;
    int          best_score;
    int          combo_pending;  /* 1 = next obstacle must be antenna (combo) */
    ObstacleType last_obstacle;  /* type of last spawned obstacle */
} Game;

void game_init(Game *g);
void game_start(Game *g);
void game_tick(Game *g, int delta_ms);
void game_input(Game *g, const InputState *input);
