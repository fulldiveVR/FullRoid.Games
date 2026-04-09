#include "render_nds.h"
#include "../../lunarrunner-common/sprites/sprite_data.h"
#include "../../lunarrunner-common/i18n/i18n.h"
#include "../../lunarrunner-common/font.h"
#include <nds.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Double-buffered framebuffer rendering */

static uint16_t *fb_back = 0;
static int fb_page = 0;

static uint16_t pal15[16];

static void init_palette(void) {
    for (int i = 0; i < 16; i++) {
        unsigned int c = sprite_palette[i];
        pal15[i] = RGB15(((c >> 16) & 0xFF) >> 3,
                         ((c >> 8)  & 0xFF) >> 3,
                          (c        & 0xFF) >> 3);
    }
}

/* --- Optimized framebuffer primitives --- */

/* Sky + ground fill via DMA — hardware fills VRAM without ARM9 bus traffic.
   Replaces separate fb_clear + draw_ground (saves one full-screen CPU pass). */
#define SKY_COLOR RGB15(0, 0, 2)

static void fb_clear_and_ground(void) {
    uint32_t sky = SKY_COLOR | ((uint32_t)SKY_COLOR << 16);
    uint32_t gnd = pal15[PAL_DKGRAY] | ((uint32_t)pal15[PAL_DKGRAY] << 16);
    dmaFillWords(sky, fb_back,                      SCREEN_W * GROUND_Y * 2);
    dmaFillWords(gnd, fb_back + SCREEN_W * GROUND_Y, SCREEN_W * (SCREEN_H - GROUND_Y) * 2);
}

static inline void fb_put(int x, int y, uint16_t color) {
    if ((unsigned)x < (unsigned)SCREEN_W && (unsigned)y < (unsigned)SCREEN_H)
        fb_back[y * SCREEN_W + x] = color;
}

/* Optimized rect fill using 32-bit writes */
static void fb_fill_rect(int x, int y, int w, int h, uint16_t color) {
    int x1 = x, y1 = y, x2 = x + w, y2 = y + h;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > SCREEN_W) x2 = SCREEN_W;
    if (y2 > SCREEN_H) y2 = SCREEN_H;
    if (x1 >= x2 || y1 >= y2) return;

    uint32_t c2 = color | ((uint32_t)color << 16);
    int rw = x2 - x1;

    for (int dy = y1; dy < y2; dy++) {
        uint16_t *row = &fb_back[dy * SCREEN_W + x1];
        int nx = 0;
        /* Align to 32-bit boundary */
        if (rw > 0 && ((uintptr_t)row & 2)) { row[0] = color; nx = 1; }
        /* 32-bit bulk fill */
        uint32_t *row32 = (uint32_t *)&row[nx];
        int pairs = (rw - nx) >> 1;
        for (int i = 0; i < pairs; i++) row32[i] = c2;
        nx += pairs * 2;
        /* Trailing pixel */
        if (nx < rw) row[nx] = color;
    }
}

static void fb_hline(int x1, int x2, int y, uint16_t color) {
    if ((unsigned)y >= (unsigned)SCREEN_H) return;
    if (x1 < 0) x1 = 0;
    if (x2 > SCREEN_W) x2 = SCREEN_W;
    if (x1 >= x2) return;

    uint16_t *row = &fb_back[y * SCREEN_W + x1];
    int w = x2 - x1;
    int nx = 0;
    if (w > 0 && ((uintptr_t)row & 2)) { row[0] = color; nx = 1; }
    uint32_t c2 = color | ((uint32_t)color << 16);
    uint32_t *r32 = (uint32_t *)&row[nx];
    int pairs = (w - nx) >> 1;
    for (int i = 0; i < pairs; i++) r32[i] = c2;
    nx += pairs * 2;
    if (nx < w) row[nx] = color;
}

/* --- Sprite blitter --- */

static void blit_sprite(int px, int py, const unsigned char *data, int w, int h) {
    for (int r = 0; r < h; r++) {
        int sy = py + r;
        if ((unsigned)sy >= (unsigned)SCREEN_H) continue;
        uint16_t *row = &fb_back[sy * SCREEN_W];
        const unsigned char *src = &data[r * w];
        for (int c = 0; c < w; c++) {
            unsigned char idx = src[c];
            if (idx == 0) continue;
            int sx = px + c;
            if ((unsigned)sx < (unsigned)SCREEN_W)
                row[sx] = pal15[idx];
        }
    }
}

/* --- Swap --- */

void render_swap(void) {
    if (fb_page) {
        REG_DISPCNT = MODE_FB0;
        fb_back = (uint16_t *)VRAM_B;
    } else {
        REG_DISPCNT = MODE_FB0 | (1 << 18);
        fb_back = (uint16_t *)VRAM_A;
    }
    fb_page ^= 1;
}

/* Sub screen: 16-bit bitmap framebuffer for Cyrillic text */
static uint16_t *sub_fb = 0;

void render_init(void) {
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    fb_back = (uint16_t *)VRAM_B;
    fb_page = 0;
    init_palette();

    /* Sub screen: extended rotation BG in 16-bit bitmap mode */
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    sub_fb = (uint16_t *)BG_BMP_RAM_SUB(0);
}

/* ═══════════════════════════════════════════════════════════
 * Background
 * ═══════════════════════════════════════════════════════════ */

static void draw_stars(int offset) {
    int shift = offset / 10;
    for (int i = 0; i < 12; i++) {
        int base_x = (i * 137 + 43) % SCREEN_W;
        int sx = base_x - (shift % SCREEN_W);
        if (sx < 0) sx += SCREEN_W;
        int sy = (i * 89 + 17) % (GROUND_Y - 20);
        fb_put(sx, sy, pal15[PAL_WHITE]);
    }
}

/* Mountain tiling period — must match spacing between peak sets */
#define MTN_PERIOD 200

static void draw_mountains(int offset) {
    int base = GROUND_Y;
    int shift = (offset / 4) % MTN_PERIOD;
    uint16_t mc = pal15[PAL_DKGRAY];

    /* Two peaks, tiled every MTN_PERIOD px */
    for (int tile = -MTN_PERIOD; tile < SCREEN_W + MTN_PERIOD; tile += MTN_PERIOD) {
        int x0 = tile - shift;
        fb_fill_rect(x0 + 30, base - 12, 30, 12, mc);
        fb_fill_rect(x0 + 20, base - 6,  50,  6, mc);
        fb_fill_rect(x0 + 120, base - 8,  24,  8, mc);
        fb_fill_rect(x0 + 110, base - 4,  40,  4, mc);
    }
}

/* ═══════════════════════════════════════════════════════════
 * Game objects
 * ═══════════════════════════════════════════════════════════ */

/* All game objects drawn as colored rects for maximum speed.
   Per-pixel sprite blitting is too slow on NDS ARM9 at 67MHz. */

static void draw_rover(const Game *g) {
    int ry = FP_TO_INT(g->rover.y_fp);
    int rh = rover_height(&g->rover);
    int rx = g->rover.x + g->fx.screen_shake_x;
    int ty = ry - rh + g->fx.screen_shake_y;

    /* Body */
    uint16_t body = g->rover.has_shield ? pal15[PAL_BLUE] : pal15[PAL_LTGRAY];
    fb_fill_rect(rx, ty, ROVER_W, rh - 3, body);
    /* Wheels */
    if (g->rover.action == ROVER_RUN || g->rover.action == ROVER_DUCK)
        fb_fill_rect(rx + 1, ry - 3, ROVER_W - 2, 3, pal15[PAL_MIDGRAY]);
    /* Headlight */
    fb_fill_rect(rx + ROVER_W - 3, ty + 1, 2, 2, pal15[PAL_BLUE]);
    /* Antenna */
    fb_put(rx + 7, ty - 3, pal15[PAL_WHITE]);
    fb_put(rx + 7, ty - 2, pal15[PAL_MIDGRAY]);
    fb_put(rx + 7, ty - 1, pal15[PAL_MIDGRAY]);
}

static void draw_obstacles(const Game *g) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        const Obstacle *o = &g->obstacles[i];
        if (!o->active) continue;
        int ox = FP_TO_INT(o->x_fp);
        if (ox > SCREEN_W || ox + o->w < 0) continue;

        switch (o->type) {
        case OBS_CRATER:
            fb_fill_rect(ox, o->y, o->w, o->h, pal15[PAL_BLACK]);
            fb_hline(ox - 1, ox + o->w + 1, o->y - 1, pal15[PAL_DKGRAY]);
            break;
        case OBS_BOULDER:
            fb_fill_rect(ox, o->y, o->w, o->h, pal15[PAL_MIDGRAY]);
            fb_fill_rect(ox + 1, o->y + 1, o->w / 2, o->h / 3, pal15[PAL_LTGRAY]);
            break;
        case OBS_ANTENNA:
            fb_fill_rect(ox, o->y + 1, o->w, o->h - 2, pal15[PAL_LTGRAY]);
            fb_fill_rect(ox + 1, o->y, 2, o->h + 3, pal15[PAL_MIDGRAY]);
            fb_fill_rect(ox + o->w - 3, o->y, 2, o->h + 3, pal15[PAL_MIDGRAY]);
            break;
        case OBS_ALIEN_FLOWER:
            fb_fill_rect(ox + o->w/2, o->y + o->h/2, 2, o->h/2 + 2, pal15[PAL_GREEN]);
            fb_fill_rect(ox, o->y, o->w, o->h/2, pal15[PAL_GREEN]);
            fb_fill_rect(ox + 2, o->y + 1, 2, 2, pal15[PAL_WHITE]);
            break;
        }
    }
}

static void draw_collectibles(const Game *g) {
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        const Collectible *c = &g->collectibles[i];
        if (!c->active) continue;
        int cx = FP_TO_INT(c->x_fp);
        if (cx > SCREEN_W || cx + 8 < 0) continue;

        switch (c->type) {
        case COLLECT_CRYSTAL:
            fb_fill_rect(cx + 1, c->y, 6, 8, pal15[PAL_YELLOW]);
            fb_fill_rect(cx + 2, c->y + 1, 2, 2, pal15[PAL_WHITE]);
            break;
        case COLLECT_STARDUST:
            fb_fill_rect(cx, c->y, 8, 8, pal15[PAL_WHITE]);
            fb_fill_rect(cx + 2, c->y + 2, 4, 4, pal15[PAL_YELLOW]);
            break;
        case COLLECT_SHIELD:
            fb_fill_rect(cx, c->y, 8, 8, pal15[PAL_BLUE]);
            fb_fill_rect(cx + 2, c->y + 2, 4, 4, pal15[PAL_CYAN]);
            break;
        }
    }
}

static void draw_particles(const FxState *fx) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &fx->particles[i];
        if (p->life <= 0) continue;
        uint16_t c = pal15[p->color_idx < 16 ? p->color_idx : PAL_WHITE];
        fb_put(p->x, p->y, c);
        fb_put(p->x + 1, p->y, c);
    }
}

static void draw_meteor(const Game *g) {
    if (!g->meteor.active) return;
    if (g->meteor.warning_timer > 0 && ((g->meteor.warning_timer / 80) & 1)) {
        fb_hline(0, SCREEN_W, 0, pal15[PAL_ORANGE]);
        fb_hline(0, SCREEN_W, 1, pal15[PAL_ORANGE]);
    }
    if (g->meteor.rain_timer > 0) {
        uint16_t mc = pal15[PAL_ORANGE];
        for (int i = 0; i < 12; i++) {
            int mx = (i * 73 + g->meteor.rain_timer / 2) % SCREEN_W;
            int my = (i * 41 + g->meteor.rain_timer * 3) % (GROUND_Y - 10);
            fb_put(mx, my, mc);
            fb_put(mx, my + 1, mc);
            fb_put(mx, my + 2, mc);
        }
    }
}

/* 4x6 minimal font */
static const unsigned char mini_font[][6] = {
    {0x6,0x9,0x9,0x9,0x9,0x6},{0x2,0x6,0x2,0x2,0x2,0x7},
    {0x6,0x9,0x2,0x4,0x8,0xF},{0xE,0x1,0x6,0x1,0x1,0xE},
    {0x9,0x9,0xF,0x1,0x1,0x1},{0xF,0x8,0xE,0x1,0x1,0xE},
    {0x6,0x8,0xE,0x9,0x9,0x6},{0xF,0x1,0x2,0x4,0x4,0x4},
    {0x6,0x9,0x6,0x9,0x9,0x6},{0x6,0x9,0x7,0x1,0x1,0x6},
};

static void fb_draw_number(int x, int y, int num, uint16_t color) {
    if (num < 0) num = 0;
    char buf[12];
    int len = 0;
    if (num == 0) { buf[0] = 0; len = 1; }
    else { int n = num; while (n > 0 && len < 11) { buf[len++] = n % 10; n /= 10; } }
    for (int i = 0; i < len / 2; i++) { char t = buf[i]; buf[i] = buf[len-1-i]; buf[len-1-i] = t; }
    for (int i = 0; i < len; i++) {
        int d = buf[i];
        if (d < 0 || d > 9) continue;
        const unsigned char *f = mini_font[d];
        for (int r = 0; r < 6; r++) {
            unsigned char row = f[r];
            for (int c = 0; c < 4; c++)
                if (row & (1 << (3 - c))) fb_put(x + i*5 + c, y + r, color);
        }
    }
}

static void draw_overlay(void) {
    /* Scanline darkening: zero every other row — write-only, no read-modify-write.
       4× less bandwidth than a full blended pass; visually gives a dark overlay. */
    for (int y = 0; y < SCREEN_H; y += 2) {
        uint32_t *row = (uint32_t *)(fb_back + y * SCREEN_W);
        for (int i = 0; i < SCREEN_W / 2; i++) row[i] = 0;
    }
}

/* ═══════════════════════════════════════════════════════════
 * Top screen
 * ═══════════════════════════════════════════════════════════ */

void render_top(const Game *g) {
    fb_clear_and_ground();

    if (g->state == STATE_MENU) {
        draw_stars(0);
        draw_mountains(0);
        fb_fill_rect(SCREEN_W/2 - 55, SCREEN_H/2 - 18, 110, 12, pal15[PAL_DKGRAY]);
        fb_draw_number(SCREEN_W/2 - 10, SCREEN_H/2 + 10, 0, pal15[PAL_WHITE]);
        blit_sprite(SCREEN_W/2 - 12, SCREEN_H/2 + 24, &spr_rover_run1[0][0], 24, 16);
        return;
    }

    draw_stars(g->world.bg_offset_far);
    draw_mountains(g->world.bg_offset_near);
    draw_obstacles(g);
    draw_collectibles(g);
    draw_rover(g);
    draw_meteor(g);
    draw_particles(&g->fx);
    fb_draw_number(SCREEN_W - 50, 4, g->score, pal15[PAL_WHITE]);

    if (g->state == STATE_GAME_OVER) {
        draw_overlay();
        fb_fill_rect(SCREEN_W/2 - 45, SCREEN_H/2 - 14, 90, 28, pal15[PAL_DKGRAY]);
        fb_draw_number(SCREEN_W/2 - 15, SCREEN_H/2 - 8, g->score, pal15[PAL_RED]);
        fb_draw_number(SCREEN_W/2 - 15, SCREEN_H/2 + 2, g->best_score, pal15[PAL_YELLOW]);
    }
    if (g->state == STATE_PAUSED)
        draw_overlay();
}

/* ═══════════════════════════════════════════════════════════
 * Bottom screen — bitmap framebuffer with bitmap font.
 * Supports full Cyrillic via font.h glyph renderer.
 * Only redraws when data changes (dirty flag).
 * ═══════════════════════════════════════════════════════════ */

#define SUB_W BOT_SCREEN_W
#define SUB_H BOT_SCREEN_H

/* BMP16 on NDS requires bit 15 set for visible pixels */
#define SUB_ALPHA BIT(15)
#define SUB_BG   (RGB15(3,3,6) | SUB_ALPHA)
#define SUB_TXT  (RGB15(28,28,28) | SUB_ALPHA)
#define SUB_DIM  (RGB15(18,18,22) | SUB_ALPHA)
#define SUB_HI   (RGB15(28,24,4) | SUB_ALPHA)
#define SUB_BAR_BG (RGB15(6,6,10) | SUB_ALPHA)
#define SUB_BAR_FG (RGB15(10,24,10) | SUB_ALPHA)

static void sub_clear(void) {
    uint32_t c2 = SUB_BG | ((uint32_t)SUB_BG << 16);
    dmaFillWords(c2, sub_fb, SUB_W * SUB_H * 2);
}

static void sub_fill_rect(int x, int y, int w, int h, uint16_t color) {
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w > SUB_W ? SUB_W : x + w;
    int y2 = y + h > SUB_H ? SUB_H : y + h;
    uint32_t c2 = color | ((uint32_t)color << 16);
    for (int dy = y1; dy < y2; dy++) {
        uint16_t *row = &sub_fb[dy * 256 + x1];
        int rw = x2 - x1, nx = 0;
        if (rw > 0 && ((uintptr_t)row & 2)) { row[0] = color; nx = 1; }
        uint32_t *r32 = (uint32_t *)&row[nx];
        int pairs = (rw - nx) >> 1;
        for (int i = 0; i < pairs; i++) r32[i] = c2;
        nx += pairs * 2;
        if (nx < rw) row[nx] = color;
    }
}

/* Draw text at (x,y) using bitmap font */
static void sub_text(int x, int y, const char *str, uint16_t color) {
    font_draw_string_fb(sub_fb, 256, SUB_H, x, y, str, color, SUB_ALPHA);
}

static GameState last_state = STATE_MENU;
static int last_score = -1, last_best = -1, last_dist = -1;
static int last_bar = -1, last_stored = -1, last_lang = -1;
static int last_shield = -1, last_solar = -1, last_turbo = -1, last_magnet = -1;

void render_bottom(const Game *g) {
    int bar_val = g->bonus.bar;
    int stored  = g->bonus.stored;
    int cur_lang = lang_get_current();
    int shield = g->rover.has_shield;
    int solar  = g->bonus.solar_timer > 0;
    int turbo  = g->bonus.turbo_timer > 0;
    int magnet = g->bonus.magnet_timer > 0;

    if (g->state == last_state && g->score == last_score &&
        g->best_score == last_best && (g->world.distance >> 3) == (last_dist >> 3) &&
        bar_val == last_bar && stored == last_stored &&
        cur_lang == last_lang && shield == last_shield &&
        solar == last_solar && turbo == last_turbo && magnet == last_magnet)
        return;

    last_state = g->state; last_score = g->score;
    last_best = g->best_score; last_dist = g->world.distance;
    last_bar = bar_val; last_stored = stored;
    last_lang = cur_lang; last_shield = shield;
    last_solar = solar; last_turbo = turbo; last_magnet = magnet;

    sub_clear();

    int y = 6;
    sub_text(8, y, lang_str(STR_TITLE), SUB_HI);
    y += 14;

    char buf[48];
    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_SCORE), g->score);
    sub_text(8, y, buf, SUB_TXT); y += 10;

    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_BEST), g->best_score);
    sub_text(8, y, buf, SUB_DIM); y += 10;

    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_DISTANCE), g->world.distance);
    sub_text(8, y, buf, SUB_DIM); y += 14;

    /* Bonus bar */
    sub_fill_rect(8, y, SUB_W - 16, 8, SUB_BAR_BG);
    int fill_w = (SUB_W - 16) * bar_val / BONUS_BAR_MAX;
    if (fill_w > 0) sub_fill_rect(8, y, fill_w, 8, SUB_BAR_FG);
    y += 12;

    if (stored != BONUS_NONE) {
        const char *name = "???";
        if (stored == BONUS_SOLAR)  name = lang_str(STR_SOLAR);
        if (stored == BONUS_TURBO)  name = lang_str(STR_TURBO);
        if (stored == BONUS_MAGNET) name = lang_str(STR_MAGNET);
        snprintf(buf, sizeof(buf), "%s: %s (X/Y)", lang_str(STR_READY), name);
        sub_text(8, y, buf, SUB_HI);
    }
    y += 10;

    if (solar) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_SOLAR), lang_str(STR_ACTIVE));
        sub_text(8, y, buf, SUB_HI); y += 10;
    }
    if (turbo) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_TURBO), lang_str(STR_ACTIVE));
        sub_text(8, y, buf, SUB_HI); y += 10;
    }
    if (magnet) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_MAGNET), lang_str(STR_ACTIVE));
        sub_text(8, y, buf, SUB_HI); y += 10;
    }
    if (shield) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_SHIELD), lang_str(STR_ACTIVE));
        sub_text(8, y, buf, SUB_HI); y += 10;
    }

    /* State hints */
    int by = SUB_H - 22;
    switch (g->state) {
    case STATE_MENU:
        sub_text(8, by, lang_str(STR_PRESS_A_START), SUB_DIM);
        break;
    case STATE_PLAYING:
        snprintf(buf, sizeof(buf), "A/B:%s Down:%s", lang_str(STR_JUMP), lang_str(STR_DUCK));
        sub_text(8, by, buf, SUB_DIM);
        snprintf(buf, sizeof(buf), "X/Y:%s START:%s", lang_str(STR_BONUS), lang_str(STR_PAUSE));
        sub_text(8, by + 10, buf, SUB_DIM);
        break;
    case STATE_PAUSED:
        sub_text(8, by, lang_str(STR_RESUME), SUB_DIM);
        sub_text(8, by + 10, lang_str(STR_QUIT), SUB_DIM);
        break;
    case STATE_GAME_OVER:
        sub_text(8, by, lang_str(STR_GAME_OVER), SUB_TXT);
        snprintf(buf, sizeof(buf), "%s  %s", lang_str(STR_RETRY), lang_str(STR_MENU));
        sub_text(8, by + 10, buf, SUB_DIM);
        break;
    }

    /* Language */
    snprintf(buf, sizeof(buf), "L/R: %s", LANG_META[cur_lang].native_name);
    sub_text(SUB_W - 80, by + 10, buf, SUB_DIM);
}
