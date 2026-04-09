#include "font_unicode.h"
#include "../../numberslide-common/config.h"
#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ═══════════════════════════════════════════════════════════════════
 * Tile-based text console — bottom screen (identical to snake-nds)
 * ═══════════════════════════════════════════════════════════════════ */

static uint16_t *g_map = NULL;

static uint32_t utf8_next(const char **s) {
    const uint8_t *p = (const uint8_t *)*s;
    uint32_t cp;
    if (*p < 0x80) {
        cp = *p++;
    } else if ((*p & 0xE0) == 0xC0) {
        cp = (*p++ & 0x1F) << 6;
        if ((*p & 0xC0) == 0x80) cp |= (*p++ & 0x3F);
    } else if ((*p & 0xF0) == 0xE0) {
        cp = (*p++ & 0x0F) << 12;
        if ((*p & 0xC0) == 0x80) { cp |= (*p++ & 0x3F) << 6; }
        if ((*p & 0xC0) == 0x80) { cp |= (*p++ & 0x3F); }
    } else {
        cp = '?'; p++;
    }
    *s = (const char *)p;
    return cp;
}

static uint16_t glyph_tile(uint32_t cp) {
    if (cp >= 0x20 && cp <= 0x7E)
        return FONT_ASCII[cp - 0x20];
    int lo = 0, hi = (int)FONT_EXT_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (FONT_EXT[mid].cp == cp) return FONT_EXT[mid].tile;
        if (FONT_EXT[mid].cp < cp) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0;
}

void text_init(void) {
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    REG_BG3CNT_SUB = BG_TILE_BASE(0) | BG_MAP_BASE(31) | BG_32x32 | BG_COLOR_16 | BG_PRIORITY_0;
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG3_ACTIVE;

    uint16_t *tile_base = (uint16_t *)BG_TILE_RAM_SUB(0);
    memcpy(tile_base, FONT_TILES, FONT_TILE_COUNT * 32);

    g_map = (uint16_t *)BG_MAP_RAM_SUB(31);

    /* Palette 0: white on black */
    BG_PALETTE_SUB[0]  = RGB15(0, 0, 0);
    BG_PALETTE_SUB[1]  = RGB15(31, 31, 31);
    /* Palette 1: orange on black */
    BG_PALETTE_SUB[16] = RGB15(0, 0, 0);
    BG_PALETTE_SUB[17] = RGB15(28, 18, 5);
    /* Palette 2: yellow on black */
    BG_PALETTE_SUB[32] = RGB15(0, 0, 0);
    BG_PALETTE_SUB[33] = RGB15(28, 28, 5);

    text_clear();
}

void text_clear(void) {
    memset(g_map, 0, 32 * 32 * 2);
}

void text_print(int col, int row, const char *utf8, int palette) {
    if (!utf8 || row < 0 || row >= 24) return;
    uint16_t pal = (palette & 0xF) << 12;
    int x = col;
    while (*utf8 && x < 32) {
        uint32_t cp = utf8_next(&utf8);
        if (cp == 0) break;
        if (x >= 0)
            g_map[row * 32 + x] = glyph_tile(cp) | pal;
        x++;
    }
}

void text_print_row(int row, const char *utf8, int palette) {
    if (row < 0 || row >= 24) return;
    uint16_t pal = (palette & 0xF) << 12;
    int x = 0;
    if (utf8) {
        while (*utf8 && x < 32) {
            uint32_t cp = utf8_next(&utf8);
            if (cp == 0) break;
            g_map[row * 32 + x] = glyph_tile(cp) | pal;
            x++;
        }
    }
    while (x < 32) {
        g_map[row * 32 + x] = 0;
        x++;
    }
}

static char _fmt_buf[128];

void text_printf_row(int row, int palette, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(_fmt_buf, sizeof(_fmt_buf), fmt, ap);
    va_end(ap);
    text_print_row(row, _fmt_buf, palette);
}

/* ═══════════════════════════════════════════════════════════════════
 * Framebuffer digit font — for tile numbers on top screen
 * ═══════════════════════════════════════════════════════════════════ */

/* Large font: 8x10 per digit (used for 1-2 digit numbers) */
static const uint8_t DIGIT_LARGE[10][10] = {
    /* 0 */ {0x3C,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C},
    /* 1 */ {0x18,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x7E},
    /* 2 */ {0x3C,0x66,0x06,0x06,0x0C,0x18,0x30,0x60,0x60,0x7E},
    /* 3 */ {0x3C,0x66,0x06,0x06,0x1C,0x06,0x06,0x06,0x66,0x3C},
    /* 4 */ {0x0C,0x1C,0x2C,0x4C,0x4C,0x7E,0x0C,0x0C,0x0C,0x0C},
    /* 5 */ {0x7E,0x60,0x60,0x60,0x7C,0x06,0x06,0x06,0x66,0x3C},
    /* 6 */ {0x3C,0x66,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x3C},
    /* 7 */ {0x7E,0x06,0x06,0x0C,0x0C,0x18,0x18,0x18,0x18,0x18},
    /* 8 */ {0x3C,0x66,0x66,0x66,0x3C,0x66,0x66,0x66,0x66,0x3C},
    /* 9 */ {0x3C,0x66,0x66,0x66,0x3E,0x06,0x06,0x06,0x66,0x3C},
};

/* Small font: 5x7 per digit (used for 3+ digit numbers) */
static const uint8_t DIGIT_SMALL[10][7] = {
    /* 0 */ {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* 1 */ {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    /* 2 */ {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    /* 3 */ {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
    /* 4 */ {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    /* 5 */ {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    /* 6 */ {0x0E,0x11,0x10,0x1E,0x11,0x11,0x0E},
    /* 7 */ {0x1F,0x01,0x02,0x04,0x04,0x08,0x08},
    /* 8 */ {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    /* 9 */ {0x0E,0x11,0x11,0x0F,0x01,0x11,0x0E},
};

#define LARGE_W 8
#define LARGE_H 10
#define SMALL_W 5
#define SMALL_H 7

static int count_digits(int v) {
    if (v < 10)    return 1;
    if (v < 100)   return 2;
    if (v < 1000)  return 3;
    if (v < 10000) return 4;
    return 5;
}

static void extract_digits(int v, int *digits, int n) {
    for (int i = n - 1; i >= 0; i--) {
        digits[i] = v % 10;
        v /= 10;
    }
}

/* Generic number draw into buffer with arbitrary stride/bounds */
static void draw_number_generic(uint16_t *buf, int stride, int bw, int bh,
                                int cx, int cy, int value, uint16_t color) {
    if (value <= 0) return;

    int nd = count_digits(value);
    int digits[5];
    extract_digits(value, digits, nd);

    if (nd <= 2) {
        int total_w = nd * LARGE_W + (nd - 1) * 1;
        int sx = cx - total_w / 2;
        int sy = cy - LARGE_H / 2;
        for (int d = 0; d < nd; d++) {
            int ox = sx + d * (LARGE_W + 1);
            const uint8_t *glyph = DIGIT_LARGE[digits[d]];
            for (int row = 0; row < LARGE_H; row++) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < LARGE_W; col++) {
                    if (bits & (0x80 >> col)) {
                        int px = ox + col, py = sy + row;
                        if ((unsigned)px < (unsigned)bw && (unsigned)py < (unsigned)bh)
                            buf[py * stride + px] = color;
                    }
                }
            }
        }
    } else {
        int total_w = nd * SMALL_W + (nd - 1) * 1;
        int sx = cx - total_w / 2;
        int sy = cy - SMALL_H / 2;
        for (int d = 0; d < nd; d++) {
            int ox = sx + d * (SMALL_W + 1);
            const uint8_t *glyph = DIGIT_SMALL[digits[d]];
            for (int row = 0; row < SMALL_H; row++) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < SMALL_W; col++) {
                    if (bits & (0x10 >> col)) {
                        int px = ox + col, py = sy + row;
                        if ((unsigned)px < (unsigned)bw && (unsigned)py < (unsigned)bh)
                            buf[py * stride + px] = color;
                    }
                }
            }
        }
    }
}

void fb_draw_number(uint16_t *fb, int cx, int cy, int value, uint16_t color) {
    draw_number_generic(fb, SCREEN_W, SCREEN_W, SCREEN_H, cx, cy, value, color);
}

void fb_draw_number_buf(uint16_t *buf, int stride, int bw, int bh,
                        int cx, int cy, int value, uint16_t color) {
    draw_number_generic(buf, stride, bw, bh, cx, cy, value, color);
}
