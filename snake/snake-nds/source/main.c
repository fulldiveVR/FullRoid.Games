#include <nds.h>
#include <stdlib.h>
#include <time.h>
#include "render_nds.h"
#include "input_nds.h"
#include "../../snake-common/game.h"
#include "../../snake-common/i18n/i18n.h"

/* ── VBLANK IRQ: счётчик миллисекунд (~16ms/кадр при 60fps) ────────── */
volatile uint32_t g_ms = 0;
static void vblank_irq(void) { g_ms += 16; }

int main(void) {
    /* Инициализация прерываний и VBLANK */
    irqSet(IRQ_VBLANK, vblank_irq);
    irqEnable(IRQ_VBLANK);

    srand((unsigned)time(NULL));

    render_init();
    input_init();

    /* Автоопределение языка, по умолчанию EN */
    lang_set(lang_detect_system());

    static Game g;
    game_init(&g);

    uint32_t last_tick = 0;

    while (1) {
        swiWaitForVBlank();

        input_handle(&g);

        /* Игровой тик по таймеру */
        if (g.state == STATE_PLAYING) {
            uint32_t now = g_ms;
            if (now - last_tick >= (uint32_t)game_tick_ms(&g)) {
                last_tick = now;
                game_update(&g);
            }
        } else {
            last_tick = g_ms; /* сбрасываем при паузе/меню */
        }

        render_top(&g);
        render_bottom(&g);
    }

    return 0;
}
