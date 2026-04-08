#pragma once

#ifdef NDS
#  define FIELD_WIDTH    42
#  define FIELD_HEIGHT   32
#  define CELL_SIZE       6
#  define SCREEN_W      256
#  define SCREEN_H      192
#  define BOT_SCREEN_W  256
#  define BOT_SCREEN_H  192
#elif defined(__3DS__)
#  define FIELD_WIDTH    50
#  define FIELD_HEIGHT   30
#  define CELL_SIZE       8
#  define SCREEN_W      400
#  define SCREEN_H      240
#  define BOT_SCREEN_W  320
#  define BOT_SCREEN_H  240
#else
#  error "Define NDS or __3DS__"
#endif

#define INITIAL_LENGTH   3
#define WIN_LENGTH     200
#define MAX_FOOD         5
#define FOOD_EFFECT      5
#define SPEED_MIN        8   /* клеток/сек */
#define SPEED_MAX       25

/* Кнопка смены языка на нижнем экране */
#define LANG_BTN_X       8
#define LANG_BTN_H      24
#ifdef NDS
#  define LANG_BTN_Y   (BOT_SCREEN_H - LANG_BTN_H - 4)
#  define LANG_BTN_W   (BOT_SCREEN_W - LANG_BTN_X * 2)
#else
#  define LANG_BTN_Y   (BOT_SCREEN_H - LANG_BTN_H - 4)
#  define LANG_BTN_W   (BOT_SCREEN_W - LANG_BTN_X * 2)
#endif
