#include <nds.h>
#include "render_nds.h"
#include "input_nds.h"
#include "../../lunarrunner-common/game.h"
#include "../../lunarrunner-common/rng.h"
#include "../../lunarrunner-common/save.h"

int main(void) {
    /* Seed RNG from hardware timer */
    TIMER0_CR = TIMER_ENABLE;
    TIMER1_CR = TIMER_ENABLE | TIMER_CASCADE;
    rng_seed((unsigned)(TIMER0_DATA | (TIMER1_DATA << 16)));

    render_init();
    input_init();

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

        InputState input;
        input_poll_nds(&input);
        game_input(&g, &input);

        render_top(&g);
        render_bottom(&g);

        /* Wait for VBlank to end */
        while (REG_VCOUNT >= 192);
    }

    return 0;
}
