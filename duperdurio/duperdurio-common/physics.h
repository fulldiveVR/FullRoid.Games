#pragma once
#include <stdint.h>

/* Axis-aligned bounding box in screen pixels */
typedef struct {
    int x, y, w, h;
} Rect;

/* Returns 1 if two rects overlap */
int rect_overlap(const Rect *a, const Rect *b);

/* Push rect A out of rect B, resolving on the shortest axis.
   dx/dy receive the applied displacement (pixels). */
void rect_resolve(Rect *a, const Rect *b, int *dx, int *dy);
