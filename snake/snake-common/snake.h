#pragma once
#include "config.h"

typedef struct { int x, y; } Point;

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} DirIndex;

extern const Point DIRS[4];

typedef struct {
    Point    segments[FIELD_WIDTH * FIELD_HEIGHT];
    int      length;
    DirIndex direction;
    DirIndex next_direction;
    int      grow_amount;
    int      shrink_amount;
} Snake;

void snake_reset(Snake *s);
void snake_update(Snake *s);
void snake_set_direction(Snake *s, DirIndex d);
int  snake_check_wall(const Snake *s);
int  snake_check_self(const Snake *s);
int  snake_occupies(const Snake *s, int x, int y);
