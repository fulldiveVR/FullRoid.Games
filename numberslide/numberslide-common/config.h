#pragma once

#define GRID_SIZE       4
#define TILE_GAP        4
#define WIN_VALUE       2048
#define SPAWN_CHANCE_4  10      /* 10% chance to spawn 4 instead of 2 */

#ifdef NDS
#  define SCREEN_W      256
#  define SCREEN_H      192
#  define BOT_SCREEN_W  256
#  define BOT_SCREEN_H  192
#  define TILE_SIZE      40
#  define BOARD_OFFSET_X 42     /* (256 - 4*40 - 3*4) / 2 */
#  define BOARD_OFFSET_Y 10     /* (192 - 4*40 - 3*4) / 2 */
#elif defined(__3DS__)
#  define SCREEN_W      400
#  define SCREEN_H      240
#  define BOT_SCREEN_W  320
#  define BOT_SCREEN_H  240
#  define TILE_SIZE      52
#  define BOARD_OFFSET_X 90     /* (400 - 4*52 - 3*4) / 2 */
#  define BOARD_OFFSET_Y 10     /* (240 - 4*52 - 3*4) / 2 */
#else
#  error "Define NDS or __3DS__"
#endif

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} SlideDir;

/* Language button on the bottom screen */
#define LANG_BTN_X       8
#define LANG_BTN_H      24
#define LANG_BTN_Y      (BOT_SCREEN_H - LANG_BTN_H - 4)
#define LANG_BTN_W      (BOT_SCREEN_W - LANG_BTN_X * 2)
