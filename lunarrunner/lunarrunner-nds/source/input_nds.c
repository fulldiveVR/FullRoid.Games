#include "input_nds.h"
#include <nds.h>

void input_init(void) {
}

void input_poll_nds(InputState *s) {
    scanKeys();
    uint32_t down = keysDown();
    uint32_t held = keysHeld();
    uint32_t up   = keysUp();

    s->jump          = (down & (KEY_A | KEY_B)) ? 1 : 0;
    s->duck          = (held & KEY_DOWN) ? 1 : 0;
    s->duck_released = (up & KEY_DOWN) ? 1 : 0;
    s->bonus         = (down & (KEY_X | KEY_Y)) ? 1 : 0;
    s->pause         = (down & KEY_START) ? 1 : 0;
    s->confirm       = (down & KEY_A) ? 1 : 0;
    s->back          = (down & KEY_B) ? 1 : 0;
    s->lang_next     = (down & KEY_R) ? 1 : 0;
    s->lang_prev     = (down & KEY_L) ? 1 : 0;
}
