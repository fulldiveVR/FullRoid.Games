#include <3ds.h>
#include "render_3ds.h"
#include "input_3ds.h"
#include "../../duperdurio-common/game.h"
#include "../../duperdurio-common/rng.h"

int main(void) {
    gfxInitDefault();
    render_init_3ds();

    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    rng_seed((unsigned)osGetTime());

    static Game g;
    game_init(&g);

    u64 prev_ms = osGetTime();

    while (aptMainLoop()) {
        u64 now   = osGetTime();
        int delta = (int)(now - prev_ms);
        prev_ms   = now;
        if (delta > 50) delta = 50;   /* cap at 50 ms (20 fps min) */
        if (delta < 1)  delta = 1;

        InputState inp;
        input_poll_3ds(&inp);

        /* Update physics (GAME_PLAYING only; others handled in game_input) */
        game_tick(&g, delta);

        /* Process player input */
        game_input(&g,
                   inp.left, inp.right, inp.run,
                   inp.jump_down, inp.jump_held,
                   inp.pause, inp.confirm, inp.back, delta);

        render_frame_3ds(&g, top, bot);
    }

    render_exit_3ds();
    gfxExit();
    return 0;
}
