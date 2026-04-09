#include "collectible.h"

void collectible_init_all(Collectible col[]) {
    for (int i = 0; i < MAX_COLLECTIBLES; i++)
        col[i].active = 0;
}

void collectible_move_all(Collectible col[], int dx_px) {
    int dx_fp = INT_TO_FP(dx_px);
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        if (!col[i].active) continue;
        col[i].x_fp -= dx_fp;
        if (FP_TO_INT(col[i].x_fp) < -16)
            col[i].active = 0;
    }
}
