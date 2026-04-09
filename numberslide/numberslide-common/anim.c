#include "anim.h"
#include <string.h>

void anim_clear(AnimState *a) {
    memset(a, 0, sizeof(*a));
}

static void add(AnimState *a, AnimType type, int fr, int fc, int tr, int tc,
                int value, int dur) {
    if (a->count >= MAX_ANIMS) return;
    Anim *an       = &a->items[a->count++];
    an->type        = type;
    an->from_row    = fr;
    an->from_col    = fc;
    an->to_row      = tr;
    an->to_col      = tc;
    an->value       = value;
    an->elapsed_ms  = 0;
    an->duration_ms = dur;
    an->active      = 1;
}

void anim_add_slide(AnimState *a, int fr, int fc, int tr, int tc, int value) {
    if (fr == tr && fc == tc) return;  /* no movement */
    add(a, ANIM_SLIDE, fr, fc, tr, tc, value, ANIM_MS_SLIDE);
}

void anim_add_merge(AnimState *a, int row, int col, int value) {
    add(a, ANIM_MERGE, row, col, row, col, value, ANIM_MS_MERGE);
}

void anim_add_spawn(AnimState *a, int row, int col, int value) {
    add(a, ANIM_SPAWN, row, col, row, col, value, ANIM_MS_SPAWN);
}

void anim_tick(AnimState *a, int delta_ms) {
    for (int i = 0; i < a->count; i++) {
        if (!a->items[i].active) continue;
        a->items[i].elapsed_ms += delta_ms;
        if (a->items[i].elapsed_ms >= a->items[i].duration_ms)
            a->items[i].active = 0;
    }
}

int anim_busy(const AnimState *a) {
    for (int i = 0; i < a->count; i++)
        if (a->items[i].active) return 1;
    return 0;
}
