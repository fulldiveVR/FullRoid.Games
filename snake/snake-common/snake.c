#include "snake.h"
#include <string.h>

const Point DIRS[4] = {
    { 0, -1}, /* UP    */
    { 0,  1}, /* DOWN  */
    {-1,  0}, /* LEFT  */
    { 1,  0}  /* RIGHT */
};

void snake_reset(Snake *s) {
    int sx = FIELD_WIDTH  / 2;
    int sy = FIELD_HEIGHT / 2;

    s->length     = INITIAL_LENGTH;
    s->direction  = DIR_RIGHT;
    s->next_direction = DIR_RIGHT;
    s->grow_amount   = 0;
    s->shrink_amount = 0;

    for (int i = 0; i < INITIAL_LENGTH; i++) {
        s->segments[i].x = sx - i;
        s->segments[i].y = sy;
    }
}

void snake_update(Snake *s) {
    s->direction = s->next_direction;
    Point d = DIRS[s->direction];

    /* Shift segments back, prepend new head */
    for (int i = s->length; i > 0; i--)
        s->segments[i] = s->segments[i - 1];

    s->segments[0].x = s->segments[1].x + d.x;
    s->segments[0].y = s->segments[1].y + d.y;

    if (s->grow_amount > 0) {
        s->length++;
        s->grow_amount--;
    } else if (s->shrink_amount > 0 && s->length > 1) {
        /* Remove two tail segments (one for the growth step, one for shrink) */
        if (s->length > 1) s->length--;
        if (s->length > 1) s->length--;
        s->shrink_amount--;
    }
    /* If grow/shrink == 0 — tail is not appended (already shifted forward) */
}

void snake_set_direction(Snake *s, DirIndex d) {
    /* Prevent 180° U-turn */
    Point cur  = DIRS[s->direction];
    Point next = DIRS[d];
    if (cur.x + next.x == 0 && cur.y + next.y == 0) return;
    s->next_direction = d;
}

int snake_check_wall(const Snake *s) {
    int x = s->segments[0].x;
    int y = s->segments[0].y;
    return x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT;
}

int snake_check_self(const Snake *s) {
    int hx = s->segments[0].x;
    int hy = s->segments[0].y;
    for (int i = 1; i < s->length; i++) {
        if (s->segments[i].x == hx && s->segments[i].y == hy)
            return 1;
    }
    return 0;
}

int snake_occupies(const Snake *s, int x, int y) {
    for (int i = 0; i < s->length; i++) {
        if (s->segments[i].x == x && s->segments[i].y == y)
            return 1;
    }
    return 0;
}
