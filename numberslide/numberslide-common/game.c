#include "game.h"
#include "save.h"

void game_init(Game *g) {
    board_init(&g->board);
    anim_clear(&g->anim);
    g->state         = STATE_MENU;
    g->best_score    = 0;
    g->continued     = 0;
    g->spawn_pending = 0;
}

void game_start(Game *g) {
    board_init(&g->board);
    anim_clear(&g->anim);
    g->state         = STATE_PLAYING;
    g->continued     = 0;
    g->spawn_pending = 0;
    board_spawn(&g->board);
    board_spawn(&g->board);
}

void game_move(Game *g, SlideDir dir) {
    if (g->state != STATE_PLAYING) return;

    if (!board_slide(&g->board, dir, &g->anim))
        return; /* nothing moved */

    if (g->board.score > g->best_score) {
        g->best_score = g->board.score;
        save_write(g->best_score);
    }

    /* Defer spawn until slide animation finishes */
    g->spawn_pending = 1;
}

void game_tick(Game *g, int delta_ms) {
    anim_tick(&g->anim, delta_ms);

    /* When slide animations finish, do the deferred spawn */
    if (g->spawn_pending && !anim_busy(&g->anim)) {
        g->spawn_pending = 0;

        anim_clear(&g->anim);
        board_spawn_anim(&g->board, &g->anim);

        /* Check win/lose after spawn */
        if (board_max_value(&g->board) >= WIN_VALUE && !g->continued) {
            g->state = STATE_WIN;
            return;
        }
        if (!board_has_moves(&g->board))
            g->state = STATE_GAME_OVER;
    }
}

int game_anim_busy(const Game *g) {
    return anim_busy(&g->anim) || g->spawn_pending;
}
