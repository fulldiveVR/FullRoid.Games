#pragma once
#include "config.h"

typedef enum {
    OBS_CRATER,
    OBS_BOULDER,
    OBS_ANTENNA,
    OBS_ALIEN_FLOWER
} ObstacleType;

typedef struct {
    ObstacleType type;
    int          x_fp;
    int          y;
    int          w, h;
    int          active;
    int          variant;
} Obstacle;

#define MAX_OBSTACLES 8

typedef struct {
    int  active;
    int  warning_timer;
    int  rain_timer;
} MeteorRain;

void obstacle_init_all(Obstacle obs[], MeteorRain *meteor);
void obstacle_move_all(Obstacle obs[], int dx_px);

/* Spawn one obstacle. Returns pointer to spawned obstacle or NULL if full. */
Obstacle *obstacle_spawn(Obstacle obs[], ObstacleType type, int variant);

void meteor_init(MeteorRain *m);
void meteor_tick(MeteorRain *m, int delta_ms);
