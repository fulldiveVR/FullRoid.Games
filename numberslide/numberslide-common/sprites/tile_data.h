#pragma once

/*
 * Tile color palette for NumberSlide.
 * Colors stored as 24-bit RGB (0xRRGGBB).
 * Platform renderers convert to native format:
 *   NDS:  RGB555  — (r>>3) | ((g>>3)<<5) | ((b>>3)<<10) | BIT(15)
 *   3DS:  RGBA8   — via C2D_Color32(r, g, b, 0xFF)
 */

#define BOARD_BG_COLOR  0xBBADA0

typedef struct {
    int  value;         /* tile value (0 = empty) */
    unsigned int bg;    /* background color RGB24 */
    unsigned int fg;    /* text color RGB24 (0 for empty) */
} TileStyle;

static const TileStyle TILE_STYLES[] = {
    {    0, 0xCDC1B4, 0x000000 },  /* empty */
    {    2, 0xEEE4DA, 0x776E65 },
    {    4, 0xEDE0C8, 0x776E65 },
    {    8, 0xF2B179, 0xF9F6F2 },
    {   16, 0xF59563, 0xF9F6F2 },
    {   32, 0xF67C5F, 0xF9F6F2 },
    {   64, 0xF65E3B, 0xF9F6F2 },
    {  128, 0xEDCF72, 0xF9F6F2 },
    {  256, 0xEDCC61, 0xF9F6F2 },
    {  512, 0xEDC850, 0xF9F6F2 },
    { 1024, 0xEDC53F, 0xF9F6F2 },
    { 2048, 0xEDC22E, 0xF9F6F2 },
};

#define TILE_STYLE_COUNT ((int)(sizeof(TILE_STYLES) / sizeof(TILE_STYLES[0])))

/* Fallback for values > 2048 */
#define TILE_SUPER_BG  0x3C3A32
#define TILE_SUPER_FG  0xF9F6F2

static inline const TileStyle *tile_style_for(int value) {
    for (int i = 0; i < TILE_STYLE_COUNT; i++)
        if (TILE_STYLES[i].value == value)
            return &TILE_STYLES[i];
    return (void *)0; /* caller should use TILE_SUPER_{BG,FG} */
}

/* ── NDS helper: RGB24 → RGB555 ── */

#define RGB24_TO_555(c) \
    ((((c) >> 19) & 0x1F) | ((((c) >> 11) & 0x1F) << 5) | ((((c) >> 3) & 0x1F) << 10) | (1 << 15))
