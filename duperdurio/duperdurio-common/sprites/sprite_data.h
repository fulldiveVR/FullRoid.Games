#pragma once
#include <stdint.h>

/*
 * Sprite palette and tile indices for DuperDurio.
 *
 * All sprites are described as sets of colored rectangles drawn via
 * C2D_DrawRectSolid — no external texture files required.
 * This mirrors the lunarrunner approach: shapes built from primitives.
 *
 * When real sprite sheets are available (exported from the JS source
 * via grit), replace the draw_* functions in render_3ds.c with
 * C2D_DrawImageAt calls using these indices.
 *
 * Palette (16 colors, shared):
 *   0  transparent
 *   1  skin      
 *   2  red       
 *   3  brown     
 *   4  blue      
 *   5  yellow    
 *   6  orange    
 *   7  beige     
 *   8  green     
 *   9  dark green
 *  10  black     
 *  11  white     
 *  12  gray      
 *  13  dark gray 
 *  14  sky blue  
 *  15  gold      
 */

/* Sprite tile IDs — used as logical indices into animation frames */
#define SPR_DURIO_SMALL_STAND   0
#define SPR_DURIO_SMALL_WALK0   1
#define SPR_DURIO_SMALL_WALK1   2
#define SPR_DURIO_SMALL_WALK2   3
#define SPR_DURIO_SMALL_JUMP    4
#define SPR_DURIO_BIG_STAND     5
#define SPR_DURIO_BIG_WALK0     6
#define SPR_DURIO_BIG_WALK1     7
#define SPR_DURIO_BIG_WALK2     8
#define SPR_DURIO_BIG_JUMP      9
#define SPR_DURIO_DEAD          10

#define SPR_CRAB_WALK0        11
#define SPR_CRAB_WALK1        12
#define SPR_CRAB_SQUISH       13

#define SPR_SNAIL_WALK0         14
#define SPR_SNAIL_WALK1         15
#define SPR_SNAIL_SHELL         16

#define SPR_NUT_0              17
#define SPR_NUT_1              18
#define SPR_NUT_2              19

#define SPR_TILE_GROUND         20
#define SPR_TILE_BRICK          21
#define SPR_TILE_QBLOCK         22
#define SPR_TILE_QBLOCK_USED    23
#define SPR_TILE_PIPE_TOP       24
#define SPR_TILE_PIPE_BODY      25

/* Number of sprites */
#define SPR_COUNT               26
