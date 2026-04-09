#include "input_3ds.h"
#include "../../numberslide-common/swipe.h"
#include <3ds.h>

static int touch_active = 0;
static int touch_sx, touch_sy;

void input_handle_3ds(Game *g) {
    hidScanInput();
    u32 down = hidKeysDown();
    u32 up   = hidKeysUp();

    /* ── Touchscreen swipe detection ── */
    if (down & KEY_TOUCH) {
        touchPosition tp;
        hidTouchRead(&tp);
        touch_active = 1;
        touch_sx = tp.px;
        touch_sy = tp.py;
    }

    if ((up & KEY_TOUCH) && touch_active) {
        touchPosition tp;
        hidTouchRead(&tp);
        int dir = swipe_detect(touch_sx, touch_sy, tp.px, tp.py);
        if (dir >= 0 && g->state == STATE_PLAYING && !game_anim_busy(g))
            game_move(g, (SlideDir)dir);
        touch_active = 0;
    }

    /* ── State-dependent button handling ── */
    switch (g->state) {
    case STATE_MENU:
        if (down & (KEY_A | KEY_START | KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT))
            game_start(g);
        break;

    case STATE_PLAYING:
        if (!game_anim_busy(g)) {
            /* D-pad */
            if (down & KEY_DUP)    game_move(g, DIR_UP);
            if (down & KEY_DDOWN)  game_move(g, DIR_DOWN);
            if (down & KEY_DLEFT)  game_move(g, DIR_LEFT);
            if (down & KEY_DRIGHT) game_move(g, DIR_RIGHT);

            /* Circle pad */
            {
                circlePosition cp;
                hidCircleRead(&cp);
                if      (cp.dy >  60) { if (!(down & KEY_DUP))    game_move(g, DIR_UP);    }
                else if (cp.dy < -60) { if (!(down & KEY_DDOWN))  game_move(g, DIR_DOWN);  }
                else if (cp.dx < -60) { if (!(down & KEY_DLEFT))  game_move(g, DIR_LEFT);  }
                else if (cp.dx >  60) { if (!(down & KEY_DRIGHT)) game_move(g, DIR_RIGHT); }
            }
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

    /* ── Language — L/R (always active) ── */
    if (down & KEY_R)
        lang_set((lang_get_current() + 1) % LANG_COUNT);
    if (down & KEY_L)
        lang_set((lang_get_current() + LANG_COUNT - 1) % LANG_COUNT);
}
