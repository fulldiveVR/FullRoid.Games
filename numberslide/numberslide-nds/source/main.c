#include <nds.h>
#include "render_nds.h"
#include "input_nds.h"
#include "../../numberslide-common/game.h"
#include "../../numberslide-common/rng.h"
#include "../../numberslide-common/anim.h"
#include "../../numberslide-common/i18n/i18n.h"
#include "../../numberslide-common/save.h"

int main(void) {
    /* Seed RNG from hardware timer */
    TIMER0_CR = TIMER_ENABLE;
    TIMER1_CR = TIMER_ENABLE | TIMER_CASCADE;
    rng_seed((unsigned)(TIMER0_DATA | (TIMER1_DATA << 16)));

    render_init();
    input_init();

    lang_set(lang_detect_system());

    static Game g;
    game_init(&g);
    save_load(&g.best_score);

    /* First frame */
    render_top(&g);
    render_bottom(&g);

    for (;;) {
        /* Wait for VBlank start */
        while (REG_VCOUNT != 192);
        render_swap();

        /* ~17ms per frame at 60 FPS */
        game_tick(&g, 17);

        input_handle(&g);
        render_top(&g);
        render_bottom(&g);

        /* Wait for VBlank to end */
        while (REG_VCOUNT >= 192);
    }

    return 0;
}
