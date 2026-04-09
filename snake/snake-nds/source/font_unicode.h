#pragma once
#include <stdint.h>

/* font_data.c (auto-generated) */
extern const uint8_t FONT_TILES[];
extern const unsigned int FONT_TILE_COUNT;
extern const uint16_t FONT_ASCII[95];

typedef struct { uint16_t cp; uint16_t tile; } FontExtEntry;
extern const FontExtEntry FONT_EXT[];
extern const unsigned int FONT_EXT_COUNT;

/* Initialize tile-based text console on sub screen */
void text_init(void);

/* Clear all text */
void text_clear(void);

/* Print UTF-8 string at tile position (col, row) with palette (0=white, 1=red, 2=green) */
void text_print(int col, int row, const char *utf8, int palette);

/* Print padded to 32 chars (full row) */
void text_print_row(int row, const char *utf8, int palette);

/* Like snprintf + text_print_row */
void text_printf_row(int row, int palette, const char *fmt, ...);
