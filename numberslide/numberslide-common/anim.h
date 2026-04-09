#pragma once
#include "config.h"

#define MAX_ANIMS    (GRID_SIZE * GRID_SIZE * 2 + 1)

#ifdef NDS
/* NDS: ~5 frames at 60 FPS */
#  define ANIM_MS_SLIDE   80
#  define ANIM_MS_MERGE   50
#  define ANIM_MS_SPAWN   50
#else
/* 3DS: smoother animations */
#  define ANIM_MS_SLIDE   100
#  define ANIM_MS_MERGE    80
#  define ANIM_MS_SPAWN    80
#endif

typedef enum { ANIM_SLIDE, ANIM_MERGE, ANIM_SPAWN } AnimType;

typedef struct {
    AnimType type;
    int from_row, from_col;
    int to_row,   to_col;
    int value;
    int elapsed_ms;
    int duration_ms;
    int active;
} Anim;

typedef struct {
    Anim items[MAX_ANIMS];
    int  count;
} AnimState;

void anim_clear(AnimState *a);
void anim_add_slide(AnimState *a, int fr, int fc, int tr, int tc, int value);
void anim_add_merge(AnimState *a, int row, int col, int value);
void anim_add_spawn(AnimState *a, int row, int col, int value);
void anim_tick(AnimState *a, int delta_ms);
int  anim_busy(const AnimState *a);
