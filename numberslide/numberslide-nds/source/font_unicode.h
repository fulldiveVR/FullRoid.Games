#pragma once
#include <stdint.h>

/* font_data.c (auto-generated, copied from snake-nds) */
extern const uint8_t FONT_TILES[];
extern const unsigned int FONT_TILE_COUNT;
extern const uint16_t FONT_ASCII[95];

typedef struct { uint16_t cp; uint16_t tile; } FontExtEntry;
extern const FontExtEntry FONT_EXT[];
extern const unsigned int FONT_EXT_COUNT;

/* ── Tile-based text console (bottom screen) ── */

void text_init(void);
void text_clear(void);
void text_print(int col, int row, const char *utf8, int palette);
void text_print_row(int row, const char *utf8, int palette);
void text_printf_row(int row, int palette, const char *fmt, ...);

/* ── Framebuffer digit font (top screen tiles) ── */

/* Draw a decimal number centered in a rect on the framebuffer (SCREEN_W stride).
 * Large font (8x10) for 1-2 digits, small font (5x7) for 3+ digits. */
void fb_draw_number(uint16_t *fb, int cx, int cy, int value, uint16_t color);

/* Same but into an arbitrary buffer with given stride and bounds. */
void fb_draw_number_buf(uint16_t *buf, int stride, int bw, int bh,
                        int cx, int cy, int value, uint16_t color);
