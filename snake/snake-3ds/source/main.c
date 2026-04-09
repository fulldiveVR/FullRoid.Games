#include <3ds.h>
#include <stdlib.h>
#include "render_3ds.h"
#include "input_3ds.h"
#include "../../snake-common/game.h"
#include "../../snake-common/i18n/i18n.h"

int main(void) {
    gfxInitDefault();
    render_init_3ds();

    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    /* RNG seed */
    srand((unsigned)osGetTime());

    /* Auto-detect system language */
    lang_set(lang_detect_system());

    static Game g;
    game_init(&g);

    u64 last_tick = osGetTime();

    while (aptMainLoop()) {
        input_handle_3ds(&g);

        /* Game tick */
        if (g.state == STATE_PLAYING) {
            u64 now = osGetTime();
            if ((u32)(now - last_tick) >= (u32)game_tick_ms(&g)) {
                last_tick = now;
                game_update(&g);
            }
        } else {
            last_tick = osGetTime();
        }

        render_frame_3ds(&g, top, bot);
    }

    render_exit_3ds();
    gfxExit();
    return 0;
}
