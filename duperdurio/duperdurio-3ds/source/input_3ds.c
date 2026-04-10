#include "input_3ds.h"
#include <3ds.h>

void input_poll_3ds(InputState *s) {
    hidScanInput();
    u32 down = hidKeysDown();
    u32 held = hidKeysHeld();

    /* Left: D-pad or circle pad */
    circlePosition cp;
    hidCircleRead(&cp);

    s->left      = ((held & KEY_DLEFT)  || cp.dx < -40) ? 1 : 0;
    s->right     = ((held & KEY_DRIGHT) || cp.dx >  40) ? 1 : 0;
    s->run       = (held & KEY_Y) ? 1 : 0;
    s->jump_down = (down & (KEY_A | KEY_B)) ? 1 : 0;
    s->jump_held = (held & (KEY_A | KEY_B)) ? 1 : 0;
    s->pause     = (down & KEY_START) ? 1 : 0;
    s->confirm   = (down & KEY_A) ? 1 : 0;
    s->back      = (down & KEY_B) ? 1 : 0;
}
