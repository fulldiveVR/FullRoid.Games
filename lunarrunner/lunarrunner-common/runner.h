#pragma once
#include "config.h"

typedef enum {
    ROVER_RUN,
    ROVER_JUMP,
    ROVER_DUCK,
    ROVER_JUMP_DUCK
} RoverAction;

typedef struct {
    int          x;
    int          y_fp;
    int          vy_fp;
    RoverAction  action;
    int          has_shield;
    int          anim_frame;
    int          anim_timer;
} Rover;

void rover_init(Rover *r);
void rover_tick(Rover *r, int delta_ms);

/* Returns the current collision box top Y (integer px) */
int  rover_top(const Rover *r);
int  rover_bottom(const Rover *r);
int  rover_height(const Rover *r);
