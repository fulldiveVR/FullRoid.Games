#pragma once
#include "config.h"

typedef struct {
    int scroll_speed_fp;
    int scroll_offset_fp;
    int distance;
    int speed_timer;
    int bg_offset_far;
    int bg_offset_near;
    int bg_offset_ground;
    int spawn_distance;
} World;

void world_init(World *w);

/* Advance world by one frame. Returns dx_px (integer pixels scrolled). */
int  world_tick(World *w, int delta_ms);
