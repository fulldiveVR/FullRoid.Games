#include "food.h"
#include <stdlib.h>

static int rand_range(int n) {
    return rand() % n;
}

void food_reset(Food *f) {
    f->count               = 0;
    f->type_change_counter = 0;
}

int food_occupies(const Food *f, int x, int y) {
    for (int i = 0; i < f->count; i++) {
        if (f->items[i].x == x && f->items[i].y == y)
            return 1;
    }
    return 0;
}

void food_spawn(Food *f, const Snake *s) {
    int has_red  = 0;
    int has_blue = 0;
    for (int i = 0; i < f->count; i++) {
        if (f->items[i].type == FOOD_RED)  has_red  = 1;
        if (f->items[i].type == FOOD_BLUE) has_blue = 1;
    }

    while (f->count < MAX_FOOD) {
        int x, y, attempts = 0;
        do {
            x = rand_range(FIELD_WIDTH);
            y = rand_range(FIELD_HEIGHT);
            attempts++;
        } while ((snake_occupies(s, x, y) || food_occupies(f, x, y))
                 && attempts < 1000);

        if (attempts >= 1000) break;

        int type;
        if (!has_red) {
            type = FOOD_RED;
            has_red = 1;
        } else if (!has_blue) {
            type = FOOD_BLUE;
            has_blue = 1;
        } else {
            type = (rand_range(2) == 0) ? FOOD_RED : FOOD_BLUE;
        }

        f->items[f->count].x    = x;
        f->items[f->count].y    = y;
        f->items[f->count].type = type;
        f->count++;
    }
}

void food_update(Food *f, const Snake *s) {
    const Point dirs[4] = {{0,-1},{0,1},{-1,0},{1,0}};

    /* Случайное движение еды (20% за ход) */
    for (int i = 0; i < f->count; i++) {
        if (rand_range(5) != 0) continue;
        Point d  = dirs[rand_range(4)];
        int nx = f->items[i].x + d.x;
        int ny = f->items[i].y + d.y;
        if (nx < 0 || nx >= FIELD_WIDTH || ny < 0 || ny >= FIELD_HEIGHT) continue;
        if (snake_occupies(s, nx, ny)) continue;
        int conflict = 0;
        for (int j = 0; j < f->count; j++) {
            if (j != i && f->items[j].x == nx && f->items[j].y == ny) {
                conflict = 1; break;
            }
        }
        if (!conflict) {
            f->items[i].x = nx;
            f->items[i].y = ny;
        }
    }

    /* Смена типа одной случайной еды раз в 20 ходов */
    f->type_change_counter++;
    if (f->type_change_counter >= 20 && f->count > 0) {
        f->type_change_counter = 0;
        int idx = rand_range(f->count);
        f->items[idx].type =
            (f->items[idx].type == FOOD_RED) ? FOOD_BLUE : FOOD_RED;
    }

    /* Гарантируем минимум 1 еды каждого типа */
    int has_red = 0, has_blue = 0;
    for (int i = 0; i < f->count; i++) {
        if (f->items[i].type == FOOD_RED)  has_red  = 1;
        if (f->items[i].type == FOOD_BLUE) has_blue = 1;
    }
    if (!has_red  && f->count > 0) f->items[0].type = FOOD_RED;
    if (!has_blue && f->count > 0) f->items[f->count - 1].type = FOOD_BLUE;
}

int food_check_collision(Food *f, Point head, int *out_type) {
    for (int i = 0; i < f->count; i++) {
        if (f->items[i].x == head.x && f->items[i].y == head.y) {
            *out_type = f->items[i].type;
            /* Удаляем еду из массива */
            f->items[i] = f->items[f->count - 1];
            f->count--;
            return 1;
        }
    }
    return 0;
}
