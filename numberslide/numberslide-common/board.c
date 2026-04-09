#include "board.h"
#include "anim.h"
#include "rng.h"
#include <string.h>

void board_init(Board *b) {
    memset(b->cells, 0, sizeof(b->cells));
    b->score = 0;
}

/* ── Line slide with movement tracking ── */

/*
 * Slide a line toward index 0, tracking where each tile goes.
 *   dest[i]     = destination index for tile originally at i (-1 if empty)
 *   is_merge[j] = 1 if a merge produced the tile at position j
 * Returns 1 if anything changed.
 */
static int slide_line(int line[GRID_SIZE], int *score,
                      int dest[GRID_SIZE], int is_merge[GRID_SIZE]) {
    int before[GRID_SIZE];
    memcpy(before, line, sizeof(before));

    for (int i = 0; i < GRID_SIZE; i++) { dest[i] = -1; is_merge[i] = 0; }

    /* Compress: collect non-zeros with original indices */
    int vals[GRID_SIZE] = {0};
    int orig[GRID_SIZE];
    memset(orig, -1, sizeof(orig));
    int n = 0;
    for (int i = 0; i < GRID_SIZE; i++)
        if (line[i]) { vals[n] = line[i]; orig[n] = i; n++; }

    /* Merge pairs and record destinations */
    int out[GRID_SIZE] = {0};
    int p = 0;
    for (int i = 0; i < n; i++) {
        if (i + 1 < n && vals[i] == vals[i + 1]) {
            out[p] = vals[i] * 2;
            *score += out[p];
            dest[orig[i]]     = p;
            dest[orig[i + 1]] = p;
            is_merge[p] = 1;
            p++;
            i++;
        } else {
            out[p] = vals[i];
            dest[orig[i]] = p;
            p++;
        }
    }
    memcpy(line, out, sizeof(out));

    for (int i = 0; i < GRID_SIZE; i++)
        if (line[i] != before[i]) return 1;
    return 0;
}

/* ── Convert line index to board (row, col) ── */

static void line_to_rc(SlideDir dir, int idx, int li,
                       int *row, int *col) {
    switch (dir) {
    case DIR_LEFT:  *row = idx; *col = li; break;
    case DIR_RIGHT: *row = idx; *col = GRID_SIZE - 1 - li; break;
    case DIR_UP:    *row = li;  *col = idx; break;
    case DIR_DOWN:  *row = GRID_SIZE - 1 - li; *col = idx; break;
    }
}

/* ── Extract / insert lines ── */

static void get_line(const Board *b, SlideDir dir, int idx, int line[GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        int r = 0, c = 0;
        line_to_rc(dir, idx, i, &r, &c);
        line[i] = b->cells[r][c];
    }
}

static void set_line(Board *b, SlideDir dir, int idx, const int line[GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        int r = 0, c = 0;
        line_to_rc(dir, idx, i, &r, &c);
        b->cells[r][c] = line[i];
    }
}

/* ── Public API ── */

int board_slide(Board *b, SlideDir dir, void *anim) {
    AnimState *a = (AnimState *)anim;
    if (a) anim_clear(a);

    int moved = 0;
    for (int idx = 0; idx < GRID_SIZE; idx++) {
        int line[GRID_SIZE];
        int dest[GRID_SIZE], is_merge[GRID_SIZE];
        get_line(b, dir, idx, line);

        if (slide_line(line, &b->score, dest, is_merge)) {
            moved = 1;

            if (a) {
                /* Record SLIDE animations */
                for (int i = 0; i < GRID_SIZE; i++) {
                    if (dest[i] < 0) continue;
                    int fr = 0, fc = 0, tr = 0, tc = 0;
                    line_to_rc(dir, idx, i, &fr, &fc);
                    line_to_rc(dir, idx, dest[i], &tr, &tc);
                    /* Use the value from the OLD board for the sliding tile */
                    int r0 = 0, c0 = 0;
                    line_to_rc(dir, idx, i, &r0, &c0);
                    anim_add_slide(a, fr, fc, tr, tc, b->cells[r0][c0]);
                }
                /* Record MERGE animations */
                for (int j = 0; j < GRID_SIZE; j++) {
                    if (is_merge[j]) {
                        int mr = 0, mc = 0;
                        line_to_rc(dir, idx, j, &mr, &mc);
                        anim_add_merge(a, mr, mc, line[j]);
                    }
                }
            }
        }
        set_line(b, dir, idx, line);
    }
    return moved;
}

void board_spawn(Board *b) {
    board_spawn_anim(b, (void *)0);
}

void board_spawn_anim(Board *b, void *anim) {
    int empty[GRID_SIZE * GRID_SIZE][2];
    int count = 0;

    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            if (b->cells[r][c] == 0) {
                empty[count][0] = r;
                empty[count][1] = c;
                count++;
            }

    if (count == 0) return;

    int pick = rng_next(count);
    int val  = (rng_next(100) < SPAWN_CHANCE_4) ? 4 : 2;
    int sr = empty[pick][0], sc = empty[pick][1];
    b->cells[sr][sc] = val;

    AnimState *a = (AnimState *)anim;
    if (a) anim_add_spawn(a, sr, sc, val);
}

int board_has_moves(const Board *b) {
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++) {
            if (b->cells[r][c] == 0) return 1;
            if (c + 1 < GRID_SIZE && b->cells[r][c] == b->cells[r][c + 1]) return 1;
            if (r + 1 < GRID_SIZE && b->cells[r][c] == b->cells[r + 1][c]) return 1;
        }
    return 0;
}

int board_max_value(const Board *b) {
    int mx = 0;
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            if (b->cells[r][c] > mx) mx = b->cells[r][c];
    return mx;
}
