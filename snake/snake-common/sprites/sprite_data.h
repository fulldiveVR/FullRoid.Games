#pragma once
#include <stdint.h>

/*
 * Volumetric pixel-art sprites.
 * Indices: 0=transparent  1=highlight  2=main  3=dark  4=shadow
 *
 * Lighting from top-left, shadow at bottom-right.
 */

/* ── 8×8 (3DS, cell 8px) ────────────────────────────────────────── */

static const uint8_t SPR_FOOD_8x8[8][8] = {
    {0,0,1,1,2,2,0,0},
    {0,1,1,2,2,2,3,0},
    {1,1,2,2,2,3,3,3},
    {1,2,2,2,3,3,3,3},
    {2,2,3,3,3,3,3,3},
    {0,3,3,3,3,3,4,0},
    {0,0,3,3,3,4,4,0},
    {0,0,0,3,4,4,0,0},
};

/* Head right 8×8 (E = pupil) */
static const uint8_t SPR_HEAD_RIGHT_8x8[8][8] = {
    {0,0,1,1,2,2,0,0},
    {0,1,1,2,2,2,3,0},
    {1,1,2,2,2,3,3,3},
    {1,2,2,4,3,3,3,3},  /* pupil at position 3 */
    {2,2,3,3,3,3,3,3},
    {0,3,3,3,3,3,4,0},
    {0,0,3,3,3,4,4,0},
    {0,0,0,3,4,4,0,0},
};

/* Head left 8×8 */
static const uint8_t SPR_HEAD_LEFT_8x8[8][8] = {
    {0,0,2,2,1,1,0,0},
    {0,3,2,2,2,1,1,0},
    {3,3,3,2,2,2,1,1},
    {3,3,3,3,3,4,2,1},  /* pupil */
    {3,3,3,3,3,3,2,2},
    {0,4,3,3,3,3,3,0},
    {0,0,4,4,3,3,3,0},
    {0,0,0,4,4,3,0,0},
};

/* Head up 8×8 */
static const uint8_t SPR_HEAD_UP_8x8[8][8] = {
    {0,0,1,1,1,1,0,0},
    {0,1,1,1,2,2,2,0},
    {1,1,2,4,2,2,2,3},  /* pupil */
    {1,2,2,2,2,2,3,3},
    {2,2,2,3,3,3,3,3},
    {0,3,3,3,3,3,4,0},
    {0,0,3,3,3,4,4,0},
    {0,0,0,4,4,0,0,0},
};

/* Head down 8×8 */
static const uint8_t SPR_HEAD_DOWN_8x8[8][8] = {
    {0,0,0,4,4,0,0,0},
    {0,0,3,3,3,4,4,0},
    {0,3,3,3,3,3,4,0},
    {2,2,2,3,3,3,3,3},
    {1,2,2,2,2,2,3,3},
    {1,1,2,4,2,2,2,3},  /* pupil */
    {0,1,1,1,2,2,2,0},
    {0,0,1,1,1,1,0,0},
};

/* ── 6×6 (NDS, cell 6px) ────────────────────────────────────────── */

static const uint8_t SPR_FOOD_6x6[6][6] = {
    {0,1,2,2,3,0},
    {1,1,2,2,3,3},
    {1,2,2,2,3,3},
    {2,3,3,3,3,0},
    {0,3,3,3,4,0},
    {0,0,3,4,0,0},
};

/* Head right 6×6 */
static const uint8_t SPR_HEAD_RIGHT_6x6[6][6] = {
    {0,1,2,2,3,0},
    {1,1,2,2,3,3},
    {1,2,4,2,3,3},  /* pupil */
    {2,2,2,3,3,3},
    {0,3,3,3,4,0},
    {0,0,3,4,0,0},
};

/* Head left 6×6 */
static const uint8_t SPR_HEAD_LEFT_6x6[6][6] = {
    {0,3,2,2,1,0},
    {3,3,2,2,1,1},
    {3,3,2,4,2,1},  /* pupil */
    {3,3,3,2,2,2},
    {0,4,3,3,3,0},
    {0,0,4,3,0,0},
};

/* Head up 6×6 */
static const uint8_t SPR_HEAD_UP_6x6[6][6] = {
    {0,1,1,1,1,0},
    {1,1,2,2,2,3},
    {1,2,4,2,3,3},  /* pupil */
    {2,2,2,3,3,3},
    {0,3,3,3,4,0},
    {0,0,3,4,0,0},
};

/* Head down 6×6 */
static const uint8_t SPR_HEAD_DOWN_6x6[6][6] = {
    {0,0,3,4,0,0},
    {0,3,3,3,4,0},
    {2,2,2,3,3,3},
    {1,2,4,2,3,3},  /* pupil */
    {1,1,2,2,2,3},
    {0,1,1,1,1,0},
};

/* ── Palettes (RGBA8888, 0=transparent) ────────────────────────────── */

static const uint32_t PALETTE_RED[5] = {
    0x00000000u,  /* transparent */
    0xFF9999FFu,  /* highlight    #FF9999 */
    0x2222DDFFu,  /* main        #DD2222 */
    0x001199FFu,  /* dark        #991100 */
    0x000044FFu,  /* shadow      #440000 */
};

static const uint32_t PALETTE_BLUE[5] = {
    0x00000000u,
    0xFFCC99FFu,  /* highlight    #99CCFF */
    0xEE5522FFu,  /* main        #2255EE */
    0xAA3300FFu,  /* dark        #0033AA */
    0x551100FFu,  /* shadow      #001155 */
};

/* Green shades for the head */
static const uint32_t PALETTE_HEAD[5] = {
    0x00000000u,
    0xAAFF99FFu,  /* highlight */
    0x22BB44FFu,  /* main */
    0x007722FFu,  /* dark */
    0x003311FFu,  /* shadow */
};

/* ── RGB555 for NDS (format: ABBBBBGGGGGRRRRR) ─────────────────────
 * Macro: RGB15(r,g,b) where each channel is 0-31
 */
#define NDS_PALETTE_RED_0   0x0000u   /* transparent (key) */
#define NDS_PALETTE_RED_1   0x639Fu   /* #FF9999: r=31 g=12 b=12 */
#define NDS_PALETTE_RED_2   0x0457u   /* #DD2222: r=27 g=4  b=4  */
#define NDS_PALETTE_RED_3   0x0053u   /* #991100: r=19 g=2  b=0  */
#define NDS_PALETTE_RED_4   0x0008u   /* #440000: r=8  g=0  b=0  */

#define NDS_PALETTE_BLUE_0  0x0000u
#define NDS_PALETTE_BLUE_1  0x7ECCu   /* #99CCFF: r=12 g=12 b=31 */
#define NDS_PALETTE_BLUE_2  0x7444u   /* #2255EE: r=4  g=10 b=29 */
#define NDS_PALETTE_BLUE_3  0x5400u   /* #0033AA: r=0  g=6  b=21 */
#define NDS_PALETTE_BLUE_4  0x2800u   /* #001155: r=0  g=2  b=10 */

#define NDS_PALETTE_HEAD_0  0x0000u
#define NDS_PALETTE_HEAD_1  0x4BF5u   /* highlight green */
#define NDS_PALETTE_HEAD_2  0x0D04u   /* main            */
#define NDS_PALETTE_HEAD_3  0x03C0u   /* dark            */
#define NDS_PALETTE_HEAD_4  0x01C0u   /* shadow          */

/* Pointer to the correct head sprite by DirIndex */
#ifdef NDS
static const uint8_t (* const HEAD_SPRITES_NDS[4])[6] = {
    SPR_HEAD_UP_6x6,
    SPR_HEAD_DOWN_6x6,
    SPR_HEAD_LEFT_6x6,
    SPR_HEAD_RIGHT_6x6,
};
#else
static const uint8_t (* const HEAD_SPRITES_3DS[4])[8] = {
    SPR_HEAD_UP_8x8,
    SPR_HEAD_DOWN_8x8,
    SPR_HEAD_LEFT_8x8,
    SPR_HEAD_RIGHT_8x8,
};
#endif
