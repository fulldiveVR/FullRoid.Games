#include "input_3ds.h"
#include <3ds.h>

static int prev_duck = 0;

void input_poll_3ds(InputState *s) {
    hidScanInput();
    u32 down = hidKeysDown();
    u32 held = hidKeysHeld();

    s->jump          = (down & (KEY_A | KEY_B)) ? 1 : 0;
    s->bonus         = (down & (KEY_X | KEY_Y)) ? 1 : 0;
    s->pause         = (down & KEY_START) ? 1 : 0;
    s->confirm       = (down & KEY_A) ? 1 : 0;
    s->back          = (down & KEY_B) ? 1 : 0;

    /* Duck: D-pad down or circle pad down */
    int dpad_duck = (held & KEY_DDOWN) ? 1 : 0;

    circlePosition cp;
    hidCircleRead(&cp);
    int circle_duck = (cp.dy < -40) ? 1 : 0;

    int cur_duck = (dpad_duck || circle_duck) ? 1 : 0;
    s->duck          = cur_duck;
    s->duck_released = (prev_duck && !cur_duck) ? 1 : 0;
    prev_duck = cur_duck;

    s->lang_next     = (down & KEY_R) ? 1 : 0;
    s->lang_prev     = (down & KEY_L) ? 1 : 0;
}
