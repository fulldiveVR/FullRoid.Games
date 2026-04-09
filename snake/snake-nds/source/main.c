#include <nds.h>
#include <stdlib.h>
#include <time.h>
#include "render_nds.h"
#include "input_nds.h"
#include "../../snake-common/game.h"
#include "../../snake-common/i18n/i18n.h"

static uint32_t g_ms = 0;

int main(void) {
    srand((unsigned)time(NULL));

    render_init();
    input_init();

    lang_set(lang_detect_system());

    static Game g;
    game_init(&g);

    uint32_t last_tick = 0;

    /* First frame: render to back buffer */
    render_top(&g);
    render_bottom(&g);

    for (;;) {
        /* Wait for VBlank start — swap is instant */
        while (REG_VCOUNT != 192);
        render_swap();

        /* Now safe: LCD shows front buffer, we draw to back buffer */
        g_ms += 17;
        input_handle(&g);

        if (g.state == STATE_PLAYING) {
            uint32_t now = g_ms;
            if (now - last_tick >= (uint32_t)game_tick_ms(&g)) {
                last_tick = now;
                game_update(&g);
            }
        } else {
            last_tick = g_ms;
        }

        render_top(&g);
        render_bottom(&g);

        /* Wait for VBlank to end (so we don't swap twice) */
        while (REG_VCOUNT >= 192);
    }

    return 0;
}
