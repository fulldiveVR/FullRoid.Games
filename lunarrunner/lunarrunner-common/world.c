#include "world.h"

void world_init(World *w) {
    w->scroll_speed_fp  = SPEED_INIT;
    w->scroll_offset_fp = 0;
    w->distance         = 0;
    w->speed_timer      = 0;
    w->bg_offset_far    = 0;
    w->bg_offset_near   = 0;
    w->bg_offset_ground = 0;
    w->spawn_distance   = 0;
}

int world_tick(World *w, int delta_ms) {
    /* Fixed-point scroll */
    int dx_fp = w->scroll_speed_fp * delta_ms / 1000;
    w->scroll_offset_fp += dx_fp;
    int dx_px = FP_TO_INT(w->scroll_offset_fp);
    w->scroll_offset_fp -= INT_TO_FP(dx_px);

    w->distance       += dx_px;
    w->spawn_distance += dx_px;

    /* Parallax offsets — wrap to prevent overflow and rendering glitches.
       Each offset wraps at a value that is a multiple of all tile periods
       used in rendering (stars, mountains, ground dashes). */
    w->bg_offset_far    = (w->bg_offset_far    + dx_px) % 25600;
    w->bg_offset_near   = (w->bg_offset_near   + dx_px) % 25600;
    w->bg_offset_ground = (w->bg_offset_ground + dx_px) % 25600;

    /* Speed progression */
    w->speed_timer += delta_ms;
    if (w->speed_timer >= SPEED_INTERVAL && w->scroll_speed_fp < SPEED_MAX) {
        w->scroll_speed_fp += SPEED_INCREMENT;
        if (w->scroll_speed_fp > SPEED_MAX)
            w->scroll_speed_fp = SPEED_MAX;
        w->speed_timer -= SPEED_INTERVAL;
    }

    return dx_px;
}
