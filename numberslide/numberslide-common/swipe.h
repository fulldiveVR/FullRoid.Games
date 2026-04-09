#pragma once
#include "config.h"

#define SWIPE_MIN_DIST  12  /* minimum swipe length in pixels */

/* Returns SlideDir on success, or -1 if not a valid swipe (tap / diagonal) */
int swipe_detect(int start_x, int start_y, int end_x, int end_y);
