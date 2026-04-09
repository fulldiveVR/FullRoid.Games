#include "game.h"
#include <stdlib.h>

void game_init(Game *g) {
    g->state     = STATE_MENU;
    g->autopilot = 0;
    snake_reset(&g->snake);
    food_reset(&g->food);
}

void game_start(Game *g) {
    snake_reset(&g->snake);
    food_reset(&g->food);
    food_spawn(&g->food, &g->snake);
    g->state     = STATE_PLAYING;
    g->autopilot = 0;
}

int game_tick_ms(const Game *g) {
    int len      = g->snake.length;
    int progress = len * 100 / WIN_LENGTH;
    if (progress > 100) progress = 100;
    int speed = SPEED_MIN + (SPEED_MAX - SPEED_MIN) * progress / 100;
    return 1000 / speed;
}

/* ---- Autopilot (greedy, Manhattan) ---- */
static DirIndex autopilot_direction(const Game *g) {
    const Snake *s = &g->snake;
    const Food  *f = &g->food;
    int hx = s->segments[0].x;
    int hy = s->segments[0].y;

    /* Find the nearest blue food */
    int best_dist = 0x7FFFFFFF;
    int best_x = -1, best_y = -1;
    for (int i = 0; i < f->count; i++) {
        if (f->items[i].type != FOOD_BLUE) continue;
        int d = abs(f->items[i].x - hx) + abs(f->items[i].y - hy);
        if (d < best_dist) { best_dist = d; best_x = f->items[i].x; best_y = f->items[i].y; }
    }
    /* If no blue food — pick any */
    if (best_x < 0) {
        for (int i = 0; i < f->count; i++) {
            int d = abs(f->items[i].x - hx) + abs(f->items[i].y - hy);
            if (d < best_dist) { best_dist = d; best_x = f->items[i].x; best_y = f->items[i].y; }
        }
    }
    if (best_x < 0) return s->direction;

    /* Try all safe directions, pick the one closest to the target */
    DirIndex best_dir  = s->direction;
    int      best_dd   = 0x7FFFFFFF;
    for (int di = 0; di < 4; di++) {
        Point cur = DIRS[s->direction];
        Point nxt = DIRS[di];
        if (cur.x + nxt.x == 0 && cur.y + nxt.y == 0) continue; /* U-turn */
        int nx = hx + nxt.x;
        int ny = hy + nxt.y;
        if (nx < 0 || nx >= FIELD_WIDTH || ny < 0 || ny >= FIELD_HEIGHT) continue;
        if (snake_occupies(s, nx, ny)) continue;
        int dd = abs(nx - best_x) + abs(ny - best_y);
        if (dd < best_dd) { best_dd = dd; best_dir = (DirIndex)di; }
    }
    return best_dir;
}

void game_update(Game *g) {
    if (g->state != STATE_PLAYING) return;

    if (g->autopilot)
        snake_set_direction(&g->snake, autopilot_direction(g));

    snake_update(&g->snake);

    if (snake_check_wall(&g->snake)) { g->state = STATE_LOSE; return; }
    if (snake_check_self(&g->snake)) { g->state = STATE_LOSE; return; }

    food_update(&g->food, &g->snake);

    int eaten_type;
    if (food_check_collision(&g->food, g->snake.segments[0], &eaten_type)) {
        if (eaten_type == FOOD_BLUE)
            g->snake.grow_amount   += FOOD_EFFECT;
        else
            g->snake.shrink_amount += FOOD_EFFECT;
        food_spawn(&g->food, &g->snake);
    }

    if (g->snake.length <= 0)        { g->state = STATE_LOSE; return; }
    if (g->snake.length > WIN_LENGTH) { g->state = STATE_WIN;  return; }
}
