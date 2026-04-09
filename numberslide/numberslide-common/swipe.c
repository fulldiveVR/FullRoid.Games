#include "swipe.h"

static int iabs(int v) { return v < 0 ? -v : v; }

int swipe_detect(int start_x, int start_y, int end_x, int end_y) {
    int dx = end_x - start_x;
    int dy = end_y - start_y;
    int dist_sq = dx * dx + dy * dy;

    if (dist_sq < SWIPE_MIN_DIST * SWIPE_MIN_DIST)
        return -1;

    int adx = iabs(dx);
    int ady = iabs(dy);

    if (adx > ady * 2)
        return dx > 0 ? DIR_RIGHT : DIR_LEFT;

    if (ady > adx * 2)
        return dy > 0 ? DIR_DOWN : DIR_UP;

    return -1;  /* diagonal — neither axis dominates 2:1 */
}
