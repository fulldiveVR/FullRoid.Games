#include "runner.h"

void rover_init(Rover *r) {
    r->x          = ROVER_X_POS;
    r->y_fp       = INT_TO_FP(GROUND_Y);
    r->vy_fp      = 0;
    r->action     = ROVER_RUN;
    r->has_shield = 0;
    r->anim_frame = 0;
    r->anim_timer = 0;
}

void rover_tick(Rover *r, int delta_ms) {
    /* Animate wheels / pose */
    r->anim_timer += delta_ms;
    if (r->anim_timer >= 80) {
        r->anim_timer -= 80;
        r->anim_frame = (r->anim_frame + 1) & 3;
    }

    /* Gravity for airborne states */
    if (r->action == ROVER_JUMP || r->action == ROVER_JUMP_DUCK) {
        r->vy_fp += GRAVITY * delta_ms / 1000;
        r->y_fp  += r->vy_fp * delta_ms / 1000;

        if (FP_TO_INT(r->y_fp) >= GROUND_Y) {
            r->y_fp  = INT_TO_FP(GROUND_Y);
            r->vy_fp = 0;
            r->action = ROVER_RUN;
        }
    }
}

int rover_height(const Rover *r) {
    if (r->action == ROVER_DUCK || r->action == ROVER_JUMP_DUCK)
        return ROVER_H_DUCK;
    return ROVER_H;
}

int rover_bottom(const Rover *r) {
    return FP_TO_INT(r->y_fp);
}

int rover_top(const Rover *r) {
    return rover_bottom(r) - rover_height(r);
}
