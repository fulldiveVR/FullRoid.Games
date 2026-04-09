#include "font_unicode.h"
#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static uint16_t *g_map = NULL;

/* ── UTF-8 decoder ───────────────────────────────────────────────── */

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

/* ── Codepoint → tile index lookup ───────────────────────────────── */

static uint16_t glyph_tile(uint32_t cp) {
    /* ASCII fast path */
    if (cp >= 0x20 && cp <= 0x7E)
        return FONT_ASCII[cp - 0x20];

    /* Binary search in FONT_EXT */
    int lo = 0, hi = (int)FONT_EXT_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (FONT_EXT[mid].cp == cp) return FONT_EXT[mid].tile;
        if (FONT_EXT[mid].cp < cp) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0; /* blank for unknown */
}

/* ── Public API ──────────────────────────────────────────────────── */

void text_init(void) {
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    /* BG3 sub: text4bpp, 256x256, map at slot 31, tiles at slot 0 */
    REG_BG3CNT_SUB = BG_TILE_BASE(0) | BG_MAP_BASE(31) | BG_32x32 | BG_COLOR_16 | BG_PRIORITY_0;
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG3_ACTIVE;

    /* Copy font tiles to VRAM tile base */
    uint16_t *tile_base = (uint16_t *)BG_TILE_RAM_SUB(0);
    memcpy(tile_base, FONT_TILES, FONT_TILE_COUNT * 32);

    /* Map pointer */
    g_map = (uint16_t *)BG_MAP_RAM_SUB(31);

    /* Palette 0: white on black */
    BG_PALETTE_SUB[0]  = RGB15(0, 0, 0);
    BG_PALETTE_SUB[1]  = RGB15(31, 31, 31);
    /* Palette 1: red on black */
    BG_PALETTE_SUB[16] = RGB15(0, 0, 0);
    BG_PALETTE_SUB[17] = RGB15(28, 5, 5);
    /* Palette 2: green on black */
    BG_PALETTE_SUB[32] = RGB15(0, 0, 0);
    BG_PALETTE_SUB[33] = RGB15(5, 28, 10);

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
        if (x >= 0) {
            g_map[row * 32 + x] = glyph_tile(cp) | pal;
        }
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
    /* Pad rest with blank */
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
