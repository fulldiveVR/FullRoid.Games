#include "input_nds.h"
#include "../../numberslide-common/swipe.h"
#include <nds.h>

static int touch_active = 0;
static int touch_sx, touch_sy;

void input_init(void) {
    touch_active = 0;
}

void input_handle(Game *g) {
    scanKeys();
    uint32_t down = keysDown();
    uint32_t up   = keysUp();

    /* ── Touchscreen swipe detection ── */
    if (down & KEY_TOUCH) {
        touchPosition tp;
        touchRead(&tp);
        touch_active = 1;
        touch_sx = tp.px;
        touch_sy = tp.py;
    }

    if ((up & KEY_TOUCH) && touch_active) {
        touchPosition tp;
        touchRead(&tp);
        int dir = swipe_detect(touch_sx, touch_sy, tp.px, tp.py);
        if (dir >= 0 && g->state == STATE_PLAYING && !game_anim_busy(g))
            game_move(g, (SlideDir)dir);
        touch_active = 0;
    }

    /* Block game moves while animations play */
    int blocked = game_anim_busy(g);

    /* ── State-dependent button handling ── */
    switch (g->state) {
    case STATE_MENU:
        if (down & (KEY_A | KEY_START | KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT))
            game_start(g);
        break;

    case STATE_PLAYING:
        if (!blocked) {
            if (down & KEY_UP)    game_move(g, DIR_UP);
            if (down & KEY_DOWN)  game_move(g, DIR_DOWN);
            if (down & KEY_LEFT)  game_move(g, DIR_LEFT);
            if (down & KEY_RIGHT) game_move(g, DIR_RIGHT);
        }

        if (down & KEY_START)
            g->state = STATE_PAUSED;

        if (down & KEY_SELECT)
            game_start(g);
        break;

    case STATE_PAUSED:
        if (down & (KEY_START | KEY_A))
            g->state = STATE_PLAYING;
        if (down & KEY_SELECT)
            g->state = STATE_MENU;
        break;

    case STATE_WIN:
        if (down & KEY_A) {
            g->continued = 1;
            g->state = STATE_PLAYING;
        }
        if (down & KEY_B)
            game_start(g);
        break;

    case STATE_GAME_OVER:
        if (down & (KEY_A | KEY_START))
            game_start(g);
        if (down & KEY_B)
            g->state = STATE_MENU;
        break;
    }

    /* ── Language — L/R buttons (always active) ── */
    if (down & KEY_R)
        lang_set((lang_get_current() + 1) % LANG_COUNT);
    if (down & KEY_L)
        lang_set((lang_get_current() + LANG_COUNT - 1) % LANG_COUNT);
}
