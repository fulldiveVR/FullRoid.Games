#pragma once
#include <stdint.h>

/*
 * 5x7 bitmap font with ASCII + Cyrillic support.
 * Glyphs stored as 7 bytes per character, bits [4:0] = 5 pixel columns.
 *
 * Two rendering backends:
 *   font_draw_string_fb  — direct framebuffer write (NDS)
 *   font_draw_string_cb  — callback per horizontal run (3DS citro2d)
 */

#define FONT_W 5
#define FONT_H 7
#define FONT_SPACING 1  /* 1px between characters */

/* Returns glyph data (7 bytes) for a UTF-8 character.
   Advances *str past the consumed bytes. Returns NULL for unknown chars. */
const unsigned char *font_glyph(const char **str);

/* Draw UTF-8 string to a 16-bit framebuffer (NDS).
   Each pixel written as (color | alpha_bit). */
void font_draw_string_fb(uint16_t *fb, int fb_stride, int fb_h,
                         int x, int y, const char *str,
                         uint16_t color, uint16_t alpha_bit);

/* Callback for run-based rendering (3DS).
   Called once per horizontal run of set pixels. */
typedef void (*font_run_cb)(int x, int y, int w, void *ctx);

void font_draw_string_cb(int x, int y, const char *str,
                         font_run_cb cb, void *ctx);

/* Get string width in pixels */
int font_string_width(const char *str);
