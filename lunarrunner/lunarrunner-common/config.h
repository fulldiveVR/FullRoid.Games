#pragma once

/* --- Fixed-point arithmetic --- */
#define FP_SHIFT         8
#define FP_ONE           (1 << FP_SHIFT)
#define FP_MUL(a, b)     (((a) * (b)) >> FP_SHIFT)
#define FP_DIV(a, b)     (((a) << FP_SHIFT) / (b))
#define FP_TO_INT(a)     ((a) >> FP_SHIFT)
#define INT_TO_FP(a)     ((a) << FP_SHIFT)

/* --- Screen --- */
#ifdef NDS
#  define SCREEN_W       256
#  define SCREEN_H       192
#  define BOT_SCREEN_W   256
#  define BOT_SCREEN_H   192
#elif defined(__3DS__)
#  define SCREEN_W       400
#  define SCREEN_H       240
#  define BOT_SCREEN_W   320
#  define BOT_SCREEN_H   240
#else
#  error "Define NDS or __3DS__"
#endif

/* --- Rover --- */
#define ROVER_X_POS      (SCREEN_W / 5)
#ifdef NDS
#  define ROVER_W        24
#  define ROVER_H        16
#  define ROVER_H_DUCK   10
#elif defined(__3DS__)
#  define ROVER_W        32
#  define ROVER_H        20
#  define ROVER_H_DUCK   12
#endif
#define GROUND_Y         (SCREEN_H - 32)

/* Collision box insets — shrink hitbox relative to sprite */
#define ROVER_HIT_INSET_X  6    /* px trimmed from each side horizontally */
#define ROVER_HIT_INSET_TOP 4   /* px trimmed from top */
#define ROVER_HIT_INSET_BOT 2   /* px trimmed from bottom */

/* --- Physics (fixed-point) --- */
#define GRAVITY          INT_TO_FP(1200)
#define JUMP_VELOCITY    INT_TO_FP(-420)
#define SPEED_INIT       INT_TO_FP(80)
#define SPEED_MAX        INT_TO_FP(240)
#define SPEED_INCREMENT  INT_TO_FP(8)
#define SPEED_INTERVAL   10000

/* --- Difficulty --- */
/* Gap in ms — converted to px at runtime. Larger on 3DS due to wider screen. */
#ifdef NDS
#  define OBSTACLE_MIN_GAP_MS  900
#elif defined(__3DS__)
#  define OBSTACLE_MIN_GAP_MS  1200
#endif
#define COMBO_DISTANCE       500

/* --- Bonus bar --- */
#define BONUS_BAR_MAX    100
#define BONUS_HOLD_TIME  10000
#define BONUS_SOLAR_DUR  5000
#define BONUS_TURBO_DUR  3000
#define BONUS_MAGNET_DUR 4000
#define MAGNET_RADIUS    50

/* --- Score --- */
#define CRYSTAL_SCORE    10
#define STARDUST_SCORE   100
#define STARDUST_BAR     5
