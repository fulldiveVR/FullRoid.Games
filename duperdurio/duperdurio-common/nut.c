#include "nut.h"

#define NUT_POP_VY   INT_TO_FP(-340)
#define NUT_LIFE_MS  600   /* ms a popped nut is visible before auto-collect */

void nut_spawn(Nut *n, int wx, int wy, int from_block) {
    n->active     = 1;
    n->x_fp       = INT_TO_FP(wx);
    n->y_fp       = INT_TO_FP(wy);
    n->vy_fp      = from_block ? NUT_POP_VY : 0;
    n->anim_timer = 0;
    n->from_block = from_block;
}

void nut_update(Nut *n, int delta_ms) {
    if (!n->active) return;
    n->anim_timer += delta_ms;

    if (n->from_block) {
        /* Arc: fly up then fall */
        n->vy_fp += FP_MUL(GRAVITY, INT_TO_FP(delta_ms)) / 1000;
        n->y_fp  += FP_MUL(n->vy_fp, INT_TO_FP(delta_ms)) / 1000;

        /* Auto-remove after lifetime */
        if (n->anim_timer > NUT_LIFE_MS)
            n->active = 0;
    }
    /* Static tile nuts: no physics, just animate */
}
