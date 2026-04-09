#include <3ds.h>
#include "render_3ds.h"
#include "input_3ds.h"
#include "../../numberslide-common/game.h"
#include "../../numberslide-common/rng.h"
#include "../../numberslide-common/anim.h"
#include "../../numberslide-common/i18n/i18n.h"
#include "../../numberslide-common/save.h"

int main(void) {
    gfxInitDefault();
    render_init_3ds();

    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    rng_seed((unsigned)osGetTime());
    lang_set(lang_detect_system());

    static Game g;
    game_init(&g);
    save_load(&g.best_score);

    u64 prev_ms = osGetTime();

    while (aptMainLoop()) {
        u64 now = osGetTime();
        int delta = (int)(now - prev_ms);
        prev_ms = now;
        if (delta > 50) delta = 50; /* clamp */

        game_tick(&g, delta);

        input_handle_3ds(&g);
        render_frame_3ds(&g, top, bot);
    }

    render_exit_3ds();
    gfxExit();
    return 0;
}
