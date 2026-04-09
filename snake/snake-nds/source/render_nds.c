#include "render_nds.h"
#include "font_unicode.h"
#include "../../snake-common/sprites/sprite_data.h"
#include <nds.h>
#include <stdio.h>
#include <string.h>

/* ── Top screen: double-buffered framebuffer ─────────────────────── */

static uint16_t *fb_back = NULL;   /* back buffer (we draw here) */
static int fb_page = 0;            /* 0 = display A / draw B, 1 = display B / draw A */

/* Fast full-screen clear using 32-bit writes (2 pixels at a time) */
static void fb_clear(uint16_t color) {
    uint32_t c2 = color | ((uint32_t)color << 16);
    uint32_t *p = (uint32_t *)fb_back;
    /* 256*192 = 49152 pixels = 24576 uint32 writes */
    for (int i = 0; i < 24576; i++)
        p[i] = c2;
}

/* Inline pixel write — no bounds check (caller must ensure valid coords) */
#define FB_PUT_FAST(x, y, color) fb_back[(y) * SCREEN_W + (x)] = (color)

/* Safe pixel write with bounds check */
static inline void fb_put(int x, int y, uint16_t color) {
    if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H)
        fb_back[y * SCREEN_W + x] = color;
}

/* Draw a 6×6 sprite with an RGB555 palette — bounds-checked */
static void draw_sprite6(int x, int y,
                         const uint8_t spr[6][6],
                         const uint16_t pal[5]) {
    for (int dy = 0; dy < 6; dy++)
        for (int dx = 0; dx < 6; dx++) {
            uint8_t idx = spr[dy][dx];
            if (idx) fb_put(x + dx, y + dy, pal[idx]);
        }
}

/* 6×6 circular body: precomputed as bitmask per row.
 * Row 0: ..XX.. = 0x0C, Row 1: .XXXX. = 0x1E, Row 2-3: XXXXXX = 0x3F */
static const uint8_t BODY_MASK[6] = { 0x0C, 0x1E, 0x3F, 0x3F, 0x1E, 0x0C };

/* Draw 6×6 body circle — fast path when fully on-screen */
static void draw_body6(int x, int y, uint16_t color) {
    if (x >= 0 && x + 5 < SCREEN_W && y >= 0 && y + 5 < SCREEN_H) {
        /* Fully on-screen: no per-pixel bounds check */
        for (int dy = 0; dy < 6; dy++) {
            uint16_t *row = &fb_back[(y + dy) * SCREEN_W + x];
            uint8_t m = BODY_MASK[dy];
            for (int dx = 0; dx < 6; dx++)
                if (m & (0x20 >> dx)) row[dx] = color;
        }
    } else {
        /* Edge case: per-pixel bounds check */
        for (int dy = 0; dy < 6; dy++) {
            uint8_t m = BODY_MASK[dy];
            for (int dx = 0; dx < 6; dx++)
                if (m & (0x20 >> dx)) fb_put(x + dx, y + dy, color);
        }
    }
}

/* Snake body colour — 3-zone gradient (head=green → mid=blue → tail=red) */
static uint16_t snake_segment_color(int idx, int length) {
    int pct = (length > 1) ? (idx * 100 / (length - 1)) : 0;
    if (pct < 33)  return RGB15(5, 28, 10);  /* green */
    if (pct < 66)  return RGB15(5, 10, 28);  /* blue  */
    return             RGB15(28,  5,  5);     /* red   */
}

/* Swap front/back buffers — call once per frame during VBlank */
void render_swap(void) {
    /* DISPCNT bits 18-19 select displayed VRAM bank: 0=A, 1=B */
    if (fb_page) {
        REG_DISPCNT = MODE_FB0;                   /* display A */
        fb_back = (uint16_t *)VRAM_B;             /* draw to B */
    } else {
        REG_DISPCNT = MODE_FB0 | (1 << 18);       /* display B */
        fb_back = (uint16_t *)VRAM_A;             /* draw to A */
    }
    fb_page ^= 1;
}

/* ── Initialisation ────────────────────────────────────────────────── */
void render_init(void) {
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    fb_back = (uint16_t *)VRAM_B;  /* first frame: display A, draw to B */
    fb_page = 0;

    /* Bottom screen — custom Unicode tile console */
    text_init();
}

/* ── Top screen ────────────────────────────────────────────────────── */

static const uint16_t PAL_RED_NDS[5] = {
    0, NDS_PALETTE_RED_1, NDS_PALETTE_RED_2,
       NDS_PALETTE_RED_3, NDS_PALETTE_RED_4
};
static const uint16_t PAL_BLUE_NDS[5] = {
    0, NDS_PALETTE_BLUE_1, NDS_PALETTE_BLUE_2,
       NDS_PALETTE_BLUE_3, NDS_PALETTE_BLUE_4
};
static const uint16_t PAL_HEAD_NDS[5] = {
    0, NDS_PALETTE_HEAD_1, NDS_PALETTE_HEAD_2,
       NDS_PALETTE_HEAD_3, NDS_PALETTE_HEAD_4
};

void render_top(const Game *g) {
    /* Clear — fast 32-bit fill */
    fb_clear(RGB15(1, 1, 3));

    int fw = FIELD_WIDTH  * CELL_SIZE;
    int fh = FIELD_HEIGHT * CELL_SIZE;
    int ox = (SCREEN_W - fw) / 2;
    int oy = (SCREEN_H - fh) / 2;

    /* Food */
    for (int i = 0; i < g->food.count; i++) {
        int fx = ox + g->food.items[i].x * CELL_SIZE;
        int fy = oy + g->food.items[i].y * CELL_SIZE;
        const uint16_t *pal = (g->food.items[i].type == FOOD_RED)
                              ? PAL_RED_NDS : PAL_BLUE_NDS;
        draw_sprite6(fx, fy, SPR_FOOD_6x6, pal);
    }

    /* Snake — tail to head */
    for (int i = g->snake.length - 1; i >= 0; i--) {
        int sx = ox + g->snake.segments[i].x * CELL_SIZE;
        int sy = oy + g->snake.segments[i].y * CELL_SIZE;
        if (i == 0) {
            const uint8_t (*hspr)[6] = HEAD_SPRITES_NDS[g->snake.direction];
            draw_sprite6(sx, sy, hspr, PAL_HEAD_NDS);
        } else {
            uint16_t col = snake_segment_color(i, g->snake.length);
            draw_body6(sx, sy, col);
        }
    }
}

/* ── Bottom screen (tile-based Unicode text) ──────────────────────── */

void render_bottom(const Game *g) {
    text_clear();
    int y = 0;

    text_print_row(y, lang_str(STR_TITLE), 0);   y++;
    y++; /* blank */

    if (g->state == STATE_MENU) {
        text_printf_row(y, 0, " %s", lang_str(STR_START));    y++;
        text_printf_row(y, 0, " %s", lang_str(STR_ANY_KEY));  y++;
        y++;
        text_printf_row(y, 0, " Y: %s", lang_str(STR_AUTOPILOT)); y++;
        text_print_row(y, " L/R: language", 0);                    y++;
    } else if (g->state == STATE_WIN) {
        text_printf_row(y, 0, " %s", lang_str(STR_WIN));      y++;
        text_printf_row(y, 0, " %s", lang_str(STR_RESTART));  y++;
        text_printf_row(y, 0, " %s", lang_str(STR_ANY_KEY));  y++;
    } else if (g->state == STATE_LOSE) {
        text_printf_row(y, 0, " %s", lang_str(STR_LOSE));     y++;
        text_printf_row(y, 0, " %s", lang_str(STR_RESTART));  y++;
        text_printf_row(y, 0, " %s", lang_str(STR_ANY_KEY));  y++;
    } else {
        /* PLAYING / PAUSED */
        text_printf_row(y, 0, " %s %d/%d",
            lang_str(STR_SIZE), g->snake.length, WIN_LENGTH);  y++;

        /* Progress bar */
        {
            char bar[32];
            int filled = (WIN_LENGTH > 0) ? (g->snake.length * 20 / WIN_LENGTH) : 0;
            if (filled > 20) filled = 20;
            bar[0] = ' '; bar[1] = '[';
            for (int i = 0; i < 20; i++) bar[2+i] = (i < filled) ? '=' : ' ';
            bar[22] = ']'; bar[23] = 0;
            text_print_row(y, bar, 0);                         y++;
        }

        y++; /* blank */
        /* Food legend: red @:-5  green @:+5 */
        text_print(1, y, "@", 1);   /* red */
        text_print(2, y, ":-5   ", 0);
        text_print(8, y, "@", 2);   /* green */
        text_print(9, y, ":+5", 0);                           y++;

        if (g->state == STATE_PAUSED) {
            y++;
            text_printf_row(y, 0, " ** %s **", lang_str(STR_PAUSED)); y++;
        } else {
            y++;
        }

        text_printf_row(y, 0, " %s:%s  [Y]",
            lang_str(STR_AUTOPILOT),
            g->autopilot ? lang_str(STR_ON) : lang_str(STR_OFF));
        y++;
    }

    /* Row 22-23: language */
    text_print_row(22, "   L: prev        R: next", 0);
    text_printf_row(23, 0, " < %-24s >",
        LANG_META[lang_get_current()].native_name);
}

/* ── Language auto-detection ───────────────────────────────────────── */
/* NDS PersonalData->language: 0=JP 1=EN 2=FR 3=DE 4=IT 5=ES 6=ZH */
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
