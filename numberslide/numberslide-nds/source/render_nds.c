#include "render_nds.h"
#include "font_unicode.h"
#include "../../numberslide-common/sprites/tile_data.h"
#include "../../numberslide-common/anim.h"
#include <nds.h>
#include <stdio.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════
 * Top screen: double-buffered framebuffer
 * ═══════════════════════════════════════════════════════════════════ */

static uint16_t *fb_back = NULL;
static int fb_page = 0;

static void fb_clear(uint16_t color) {
    uint32_t c2 = color | ((uint32_t)color << 16);
    uint32_t *p = (uint32_t *)fb_back;
    for (int i = 0; i < 24576; i++)
        p[i] = c2;
}

/* Fast rect fill — no bounds check, 32-bit writes */
static void fb_fill_rect_fast(int x, int y, int w, int h, uint16_t color) {
    uint32_t c2 = color | ((uint32_t)color << 16);
    for (int dy = 0; dy < h; dy++) {
        uint16_t *row = &fb_back[(y + dy) * SCREEN_W + x];
        int nx = 0;
        if (w > 0 && ((uintptr_t)row & 2)) { row[0] = color; nx = 1; }
        uint32_t *row32 = (uint32_t *)&row[nx];
        int pairs = (w - nx) >> 1;
        for (int i = 0; i < pairs; i++) row32[i] = c2;
        nx += pairs * 2;
        if (nx < w) row[nx] = color;
    }
}

/* Clipped rect fill — for animated tiles that may be partially off-screen */
static void fb_fill_rect(int x, int y, int w, int h, uint16_t color) {
    int x1 = x, y1 = y, x2 = x + w, y2 = y + h;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > SCREEN_W) x2 = SCREEN_W;
    if (y2 > SCREEN_H) y2 = SCREEN_H;
    if (x1 >= x2 || y1 >= y2) return;
    fb_fill_rect_fast(x1, y1, x2 - x1, y2 - y1, color);
}

static inline void fb_put(int x, int y, uint16_t color) {
    if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H)
        fb_back[y * SCREEN_W + x] = color;
}

void render_swap(void) {
    if (fb_page) {
        REG_DISPCNT = MODE_FB0;
        fb_back = (uint16_t *)VRAM_B;
    } else {
        REG_DISPCNT = MODE_FB0 | (1 << 18);
        fb_back = (uint16_t *)VRAM_A;
    }
    fb_page ^= 1;
}

/* ═══════════════════════════════════════════════════════════════════
 * Pre-rendered tile sprite cache
 *   Index 0 = empty, 1 = value 2, 2 = value 4, ..., 14 = value 16384
 *   15 tiles × 40×40 × 2 bytes = ~47 KB in RAM
 * ═══════════════════════════════════════════════════════════════════ */

#define TCACHE_COUNT 15  /* empty + 2,4,8,...,16384 */

static uint16_t tile_sprites[TCACHE_COUNT][TILE_SIZE * TILE_SIZE];
static uint16_t bg_cache[TILE_STYLE_COUNT];
static uint16_t fg_cache[TILE_STYLE_COUNT];

static int tile_cache_idx(int value) {
    if (value <= 0) return 0;
    /* log2 for powers of 2: value 2→1, 4→2, 8→3, ..., 16384→14 */
    int idx = 0, v = value;
    while (v > 1) { v >>= 1; idx++; }
    return (idx >= 1 && idx < TCACHE_COUNT) ? idx : -1;
}

static uint16_t get_tile_bg(int value) {
    for (int i = 0; i < TILE_STYLE_COUNT; i++)
        if (TILE_STYLES[i].value == value) return bg_cache[i];
    return RGB24_TO_555(TILE_SUPER_BG);
}

static uint16_t get_tile_fg(int value) {
    for (int i = 0; i < TILE_STYLE_COUNT; i++)
        if (TILE_STYLES[i].value == value) return fg_cache[i];
    return RGB24_TO_555(TILE_SUPER_FG);
}

static void build_tile_cache(void) {
    /* Pre-compute color tables */
    for (int i = 0; i < TILE_STYLE_COUNT; i++) {
        bg_cache[i] = RGB24_TO_555(TILE_STYLES[i].bg);
        fg_cache[i] = RGB24_TO_555(TILE_STYLES[i].fg);
    }

    /* Render each tile value into a TILE_SIZE×TILE_SIZE buffer */
    static const int values[TCACHE_COUNT] = {
        0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
    };

    for (int t = 0; t < TCACHE_COUNT; t++) {
        uint16_t *buf = tile_sprites[t];
        int val = values[t];
        uint16_t bg = get_tile_bg(val);

        /* Fill background */
        int total = TILE_SIZE * TILE_SIZE;
        uint32_t bg2 = bg | ((uint32_t)bg << 16);
        uint32_t *p32 = (uint32_t *)buf;
        for (int i = 0; i < total / 2; i++) p32[i] = bg2;

        /* Draw number */
        if (val > 0) {
            uint16_t fg = get_tile_fg(val);
            fb_draw_number_buf(buf, TILE_SIZE, TILE_SIZE, TILE_SIZE,
                               TILE_SIZE / 2, TILE_SIZE / 2, val, fg);
        }
    }
}

/* Fast blit: copy pre-rendered tile to framebuffer — 32-bit row copies */
static void blit_tile(int px, int py, int cache_idx) {
    const uint16_t *src = tile_sprites[cache_idx];
    for (int row = 0; row < TILE_SIZE; row++) {
        uint32_t *dst32 = (uint32_t *)&fb_back[(py + row) * SCREEN_W + px];
        const uint32_t *src32 = (const uint32_t *)&src[row * TILE_SIZE];
        /* TILE_SIZE=40 → 20 uint32 copies per row */
        for (int i = 0; i < TILE_SIZE / 2; i++)
            dst32[i] = src32[i];
    }
}

/* Clipped blit for animated tiles near screen edges */
static void blit_tile_clipped(int px, int py, int cache_idx) {
    /* If fully on-screen, use fast path */
    if (px >= 0 && py >= 0 &&
        px + TILE_SIZE <= SCREEN_W && py + TILE_SIZE <= SCREEN_H) {
        blit_tile(px, py, cache_idx);
        return;
    }
    /* Slow clipped path */
    const uint16_t *src = tile_sprites[cache_idx];
    for (int row = 0; row < TILE_SIZE; row++) {
        int sy = py + row;
        if ((unsigned)sy >= SCREEN_H) continue;
        for (int col = 0; col < TILE_SIZE; col++) {
            int sx = px + col;
            if ((unsigned)sx >= SCREEN_W) continue;
            fb_back[sy * SCREEN_W + sx] = src[row * TILE_SIZE + col];
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * Coordinate helpers
 * ═══════════════════════════════════════════════════════════════════ */

static int cell_px(int col) { return BOARD_OFFSET_X + col * (TILE_SIZE + TILE_GAP); }
static int cell_py(int row) { return BOARD_OFFSET_Y + row * (TILE_SIZE + TILE_GAP); }

static int lerp_i(int a, int b, int num, int den) {
    return a + (b - a) * num / den;
}

/* Dim overlay — board area only */
static void fb_dim_board(void) {
    int bw = GRID_SIZE * TILE_SIZE + (GRID_SIZE - 1) * TILE_GAP;
    int pad = 4;
    int x0 = BOARD_OFFSET_X - pad, y0 = BOARD_OFFSET_Y - pad;
    int x1 = x0 + bw + pad * 2,   y1 = y0 + bw + pad * 2;
    for (int y = y0; y < y1; y++) {
        uint32_t *r32 = (uint32_t *)&fb_back[y * SCREEN_W + x0];
        int pairs = (x1 - x0) >> 1;
        for (int i = 0; i < pairs; i++)
            r32[i] = (r32[i] >> 1) & 0x3DEF3DEF;
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * Top screen render
 * ═══════════════════════════════════════════════════════════════════ */

/* Draw a tile that is not in the cache (value > 16384) — slow fallback */
static void draw_tile_slow(int px, int py, int value) {
    uint16_t bg = get_tile_bg(value);
    uint16_t fg = get_tile_fg(value);
    fb_fill_rect(px, py, TILE_SIZE, TILE_SIZE, bg);
    fb_draw_number(fb_back, px + TILE_SIZE / 2, py + TILE_SIZE / 2, value, fg);
}

static void draw_tile_any(int px, int py, int value) {
    int idx = tile_cache_idx(value);
    if (idx >= 0)
        blit_tile_clipped(px, py, idx);
    else
        draw_tile_slow(px, py, value);
}

void render_top(const Game *g) {
    uint16_t board_bg = RGB24_TO_555(BOARD_BG_COLOR);
    fb_clear(RGB15(1, 1, 2));

    int bw = GRID_SIZE * TILE_SIZE + (GRID_SIZE - 1) * TILE_GAP;
    int pad = 4;
    fb_fill_rect_fast(BOARD_OFFSET_X - pad, BOARD_OFFSET_Y - pad,
                      bw + pad * 2, bw + pad * 2, board_bg);

    int animating = anim_busy(&g->anim);

    if (!animating) {
        /* Static: blit cached tiles */
        for (int r = 0; r < GRID_SIZE; r++)
            for (int c = 0; c < GRID_SIZE; c++) {
                int val = g->board.cells[r][c];
                int idx = tile_cache_idx(val);
                if (idx >= 0)
                    blit_tile(cell_px(c), cell_py(r), idx);
                else if (val > 0)
                    draw_tile_slow(cell_px(c), cell_py(r), val);
                else
                    blit_tile(cell_px(c), cell_py(r), 0);
            }
    } else {
        /* Empty cell backgrounds */
        for (int r = 0; r < GRID_SIZE; r++)
            for (int c = 0; c < GRID_SIZE; c++)
                blit_tile(cell_px(c), cell_py(r), 0); /* empty sprite */

        /* Mark animated cells */
        int anim_mark[GRID_SIZE][GRID_SIZE] = {{0}};
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            anim_mark[a->to_row][a->to_col] = 1;
            if (a->type == ANIM_SLIDE)
                anim_mark[a->from_row][a->from_col] = 1;
        }

        /* Non-animated tiles */
        for (int r = 0; r < GRID_SIZE; r++)
            for (int c = 0; c < GRID_SIZE; c++)
                if (!anim_mark[r][c] && g->board.cells[r][c] > 0)
                    draw_tile_any(cell_px(c), cell_py(r), g->board.cells[r][c]);

        /* SLIDE animations */
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            if (a->type != ANIM_SLIDE) continue;
            int t = a->elapsed_ms, d = a->duration_ms;
            if (t > d) t = d;
            int px = lerp_i(cell_px(a->from_col), cell_px(a->to_col), t, d);
            int py = lerp_i(cell_py(a->from_row), cell_py(a->to_row), t, d);
            draw_tile_any(px, py, a->value);
        }

        /* MERGE — draw final tile immediately */
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            if (a->type != ANIM_MERGE) continue;
            draw_tile_any(cell_px(a->to_col), cell_py(a->to_row), a->value);
        }

        /* SPAWN — draw final tile immediately */
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            if (a->type != ANIM_SPAWN) continue;
            draw_tile_any(cell_px(a->to_col), cell_py(a->to_row), a->value);
        }
    }

    if (g->state == STATE_PAUSED || g->state == STATE_WIN ||
        g->state == STATE_GAME_OVER)
        fb_dim_board();
}

/* ═══════════════════════════════════════════════════════════════════
 * Init
 * ═══════════════════════════════════════════════════════════════════ */

void render_init(void) {
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    fb_back = (uint16_t *)VRAM_B;
    fb_page = 0;
    text_init();
    build_tile_cache();
}

/* ═══════════════════════════════════════════════════════════════════
 * Bottom screen — only redraws when dirty
 * ═══════════════════════════════════════════════════════════════════ */

static GameState last_state = STATE_MENU;
static int last_score = -1, last_best = -1;
static Language last_lang = LANG_EN;

void render_bottom(const Game *g) {
    if (g->state == last_state &&
        g->board.score == last_score &&
        g->best_score == last_best &&
        lang_get_current() == last_lang)
        return;

    last_state = g->state;
    last_score = g->board.score;
    last_best  = g->best_score;
    last_lang  = lang_get_current();

    text_clear();
    int y = 0;

    text_print_row(y, lang_str(STR_TITLE), 1);  y++;
    y++;
    text_printf_row(y, 0, " %s: %d", lang_str(STR_SCORE), g->board.score); y++;
    text_printf_row(y, 0, " %s: %d", lang_str(STR_BEST), g->best_score);   y++;
    y++;

    switch (g->state) {
    case STATE_MENU:
        text_printf_row(y, 0, " %s", lang_str(STR_ANY_KEY)); y++;
        break;
    case STATE_PLAYING:
        text_print_row(y, " D-pad / Touch: slide", 0); y++;
        text_print_row(y, " START: pause", 0);          y++;
        text_print_row(y, " SELECT: new game", 0);      y++;
        break;
    case STATE_PAUSED:
        text_printf_row(y, 1, " ** %s **", lang_str(STR_PAUSED)); y++;
        y++;
        text_print_row(y, " START/A: resume", 0);       y++;
        break;
    case STATE_WIN:
        text_printf_row(y, 2, " %s", lang_str(STR_YOU_WIN)); y++;
        y++;
        text_printf_row(y, 0, " A: %s", lang_str(STR_CONTINUE)); y++;
        text_printf_row(y, 0, " B: %s", lang_str(STR_NEW_GAME)); y++;
        break;
    case STATE_GAME_OVER:
        text_printf_row(y, 1, " %s", lang_str(STR_GAME_OVER)); y++;
        y++;
        text_printf_row(y, 0, " A: %s", lang_str(STR_NEW_GAME)); y++;
        break;
    }
    for (; y < 22; y++) text_print_row(y, "", 0);

    text_print_row(22, "   L: prev        R: next", 0);
    text_printf_row(23, 0, " < %-24s >",
        LANG_META[lang_get_current()].native_name);
}

/* ═══════════════════════════════════════════════════════════════════
 * Language auto-detection
 * ═══════════════════════════════════════════════════════════════════ */

Language lang_detect_system(void) {
    switch (PersonalData->language) {
        case 1: return LANG_EN;
        case 2: return LANG_FR;
        case 3: return LANG_DE;
        case 4: return LANG_IT;
        case 5: return LANG_ES;
        default: return LANG_EN;
    }
}
