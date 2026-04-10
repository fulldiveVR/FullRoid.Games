#include "physics.h"

int rect_overlap(const Rect *a, const Rect *b) {
    return (a->x < b->x + b->w) && (a->x + a->w > b->x) &&
           (a->y < b->y + b->h) && (a->y + a->h > b->y);
}

/*
 * Resolve AABB overlap: push A out of B on the axis with the smallest overlap.
 * Standard Durio-style: X first then Y is handled by the caller via two
 * separate calls (one for horizontal tiles, one for vertical).
 * This function just calculates the shortest-axis push.
 */
void rect_resolve(Rect *a, const Rect *b, int *dx, int *dy) {
    *dx = 0;
    *dy = 0;
    if (!rect_overlap(a, b)) return;

    int over_x_left  = (b->x + b->w) - a->x;
    int over_x_right = (a->x + a->w) - b->x;
    int over_y_top   = (b->y + b->h) - a->y;
    int over_y_bot   = (a->y + a->h) - b->y;

    int min_x = (over_x_left < over_x_right) ? over_x_left : over_x_right;
    int min_y = (over_y_top  < over_y_bot)   ? over_y_top  : over_y_bot;

    if (min_x < min_y) {
        if (over_x_left < over_x_right) {
            *dx = over_x_left;
            a->x += *dx;
        } else {
            *dx = -over_x_right;
            a->x += *dx;
        }
    } else {
        if (over_y_top < over_y_bot) {
            *dy = over_y_top;
            a->y += *dy;
        } else {
            *dy = -over_y_bot;
            a->y += *dy;
        }
    }
}
