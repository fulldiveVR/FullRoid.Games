#include "input_nds.h"
#include <nds.h>

void input_init(void) {
    /* libnds initialises input automatically */
}

void input_handle(Game *g) {
    scanKeys();
    uint32_t down = keysDown();
    uint32_t held = keysHeld();

    /* Movement — only while playing */
    if (g->state == STATE_PLAYING) {
        if (held & KEY_UP)    snake_set_direction(&g->snake, DIR_UP);
        if (held & KEY_DOWN)  snake_set_direction(&g->snake, DIR_DOWN);
        if (held & KEY_LEFT)  snake_set_direction(&g->snake, DIR_LEFT);
        if (held & KEY_RIGHT) snake_set_direction(&g->snake, DIR_RIGHT);
    }

    /* Start game — any button except L/R (those cycle language).
     * Use else-if so the pause check doesn't fire in the same frame
     * as game_start() transitions the state to STATE_PLAYING. */
    if (g->state == STATE_MENU || g->state == STATE_WIN ||
        g->state == STATE_LOSE) {
        uint32_t any = KEY_START | KEY_A | KEY_B | KEY_X | KEY_Y |
                       KEY_UP    | KEY_DOWN | KEY_LEFT | KEY_RIGHT;
        if (down & any)
            game_start(g);
    } else if (g->state == STATE_PLAYING) {
        if (down & KEY_START)
            g->state = STATE_PAUSED;
    } else if (g->state == STATE_PAUSED) {
        if (down & (KEY_START | KEY_A | KEY_B | KEY_X))
            g->state = STATE_PLAYING;
    }

    /* Autopilot toggle — Y button */
    if (g->state == STATE_PLAYING || g->state == STATE_PAUSED) {
        if (down & KEY_Y)
            g->autopilot = !g->autopilot;
    }

    /* Language — L = previous, R = next */
    if (down & KEY_R)
        lang_set((lang_get_current() + 1) % LANG_COUNT);
    if (down & KEY_L)
        lang_set((lang_get_current() + LANG_COUNT - 1) % LANG_COUNT);

    /* Touchscreen — language button area */
    if (down & KEY_TOUCH) {
        touchPosition tp;
        touchRead(&tp);
        if (tp.px >= LANG_BTN_X &&
            tp.px <  LANG_BTN_X + LANG_BTN_W &&
            tp.py >= LANG_BTN_Y &&
            tp.py <  LANG_BTN_Y + LANG_BTN_H) {
            lang_set((lang_get_current() + 1) % LANG_COUNT);
        }
    }
}
