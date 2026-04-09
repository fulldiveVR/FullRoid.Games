#pragma once
#include "config.h"

typedef struct {
    int cells[GRID_SIZE][GRID_SIZE]; /* 0 = empty, 2/4/8/.../2048/... */
    int score;
} Board;

void board_init(Board *b);
int  board_slide(Board *b, SlideDir dir, void *anim);  /* returns 1 if anything moved; anim: AnimState* or NULL */
void board_spawn(Board *b);                             /* place a random 2 or 4 on an empty cell */
void board_spawn_anim(Board *b, void *anim);            /* same, but records SPAWN animation */
int  board_has_moves(const Board *b);
int  board_max_value(const Board *b);
