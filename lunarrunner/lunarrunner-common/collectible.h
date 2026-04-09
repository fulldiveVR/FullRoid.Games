#pragma once
#include "config.h"

typedef enum {
    COLLECT_CRYSTAL,
    COLLECT_STARDUST,
    COLLECT_SHIELD
} CollectibleType;

typedef struct {
    CollectibleType type;
    int             x_fp;
    int             y;
    int             active;
} Collectible;

#define MAX_COLLECTIBLES 16

void collectible_init_all(Collectible col[]);
void collectible_move_all(Collectible col[], int dx_px);
