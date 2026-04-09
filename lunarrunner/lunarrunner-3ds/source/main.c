#include <3ds.h>
#include "render_3ds.h"
#include "input_3ds.h"
#include "../../lunarrunner-common/game.h"
#include "../../lunarrunner-common/rng.h"
#include "../../lunarrunner-common/save.h"

int main(void) {
    gfxInitDefault();
    render_init_3ds();

    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    rng_seed((unsigned)osGetTime());

    static Game g;
    game_init(&g);
    save_load(&g.best_score);

    u64 prev_ms = osGetTime();

    while (aptMainLoop()) {
        u64 now = osGetTime();
        int delta = (int)(now - prev_ms);
        prev_ms = now;
        if (delta > 50) delta = 50;

        game_tick(&g, delta);

        InputState input;
        input_poll_3ds(&input);
        game_input(&g, &input);

        render_frame_3ds(&g, top, bot);
    }

    render_exit_3ds();
    gfxExit();
    return 0;
}
