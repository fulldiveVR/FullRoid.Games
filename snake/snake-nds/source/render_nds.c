#include "render_nds.h"
#include "../../snake-common/sprites/sprite_data.h"
#include <nds.h>
#include <stdio.h>
#include <string.h>

/* ── Top screen: framebuffer Mode 5 ───────────────────────────────── */

static uint16_t *fb_top = NULL;

static inline void fb_put(int x, int y, uint16_t color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    fb_top[y * SCREEN_W + x] = color;
}

static void fb_fill_rect(int x, int y, int w, int h, uint16_t color) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            fb_put(x + dx, y + dy, color);
}

/* Draw a 6×6 sprite with an RGB555 palette */
static void draw_sprite6(int x, int y,
                         const uint8_t spr[6][6],
                         const uint16_t pal[5]) {
    for (int dy = 0; dy < 6; dy++)
        for (int dx = 0; dx < 6; dx++) {
            uint8_t idx = spr[dy][dx];
            if (idx == 0) continue;
            fb_put(x + dx, y + dy, pal[idx]);
        }
}

/* Snake body colour — 3-zone gradient (head=green → mid=blue → tail=red) */
static uint16_t snake_segment_color(int idx, int length) {
    int pct = (length > 1) ? (idx * 100 / (length - 1)) : 0;
    if (pct < 33)  return RGB15(5, 28, 10);  /* green */
    if (pct < 66)  return RGB15(5, 10, 28);  /* blue  */
    return             RGB15(28,  5,  5);     /* red   */
}

/* ── Initialisation ────────────────────────────────────────────────── */
void render_init(void) {
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    fb_top = (uint16_t *)VRAM_A;

    /* Bottom screen — text console via libnds */
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
}

/* ── Top screen ────────────────────────────────────────────────────── */

static const uint16_t PAL_RED_NDS[5] = {
    0, NDS_PALETTE_RED_1, NDS_PALETTE_RED_2,
       NDS_PALETTE_RED_3, NDS_PALETTE_RED_4
};
static const uint16_t PAL_BLUE_NDS[5] = {
    0, NDS_PALETTE_BLUE_1, NDS_PALETTE_BLUE_2,
       NDS_PALETTE_BLUE_3, NDS_PALETTE_BLUE_4
};
static const uint16_t PAL_HEAD_NDS[5] = {
    0, NDS_PALETTE_HEAD_1, NDS_PALETTE_HEAD_2,
       NDS_PALETTE_HEAD_3, NDS_PALETTE_HEAD_4
};

void render_top(const Game *g) {
    /* Clear */
    fb_fill_rect(0, 0, SCREEN_W, SCREEN_H, RGB15(1, 1, 3));

    int fw = FIELD_WIDTH  * CELL_SIZE;
    int fh = FIELD_HEIGHT * CELL_SIZE;
    int ox = (SCREEN_W - fw) / 2;
    int oy = (SCREEN_H - fh) / 2;

    /* Food */
    for (int i = 0; i < g->food.count; i++) {
        int fx = ox + g->food.items[i].x * CELL_SIZE;
        int fy = oy + g->food.items[i].y * CELL_SIZE;
        const uint16_t *pal = (g->food.items[i].type == FOOD_RED)
                              ? PAL_RED_NDS : PAL_BLUE_NDS;
        draw_sprite6(fx, fy, SPR_FOOD_6x6, pal);
    }

    /* Snake — tail to head */
    for (int i = g->snake.length - 1; i >= 0; i--) {
        int sx = ox + g->snake.segments[i].x * CELL_SIZE;
        int sy = oy + g->snake.segments[i].y * CELL_SIZE;
        if (i == 0) {
            const uint8_t (*hspr)[6] = HEAD_SPRITES_NDS[g->snake.direction];
            draw_sprite6(sx, sy, hspr, PAL_HEAD_NDS);
        } else {
            uint16_t col = snake_segment_color(i, g->snake.length);
            fb_fill_rect(sx, sy, CELL_SIZE, CELL_SIZE, col);
        }
    }
}

/* ── Bottom screen (libnds text console, 32×24 chars) ─────────────── */

/* ASCII progress bar */
static void draw_progress(int val, int max, int width) {
    int filled = (max > 0) ? (val * width / max) : 0;
    if (filled > width) filled = width;
    iprintf("[");
    for (int i = 0; i < width; i++)
        iprintf("%c", (i < filled) ? '=' : ' ');
    iprintf("]");
}

void render_bottom(const Game *g) {
    consoleClear();

    /* Console: 32 cols x 24 rows.
     * Language section is always at rows 20-21.
     * We count printed lines and pad with blank lines rather than
     * relying on cursor-position escapes (unreliable when content overflows). */

    iprintf(" %s\n\n", lang_str(STR_TITLE));   /* rows 0-1 */
    int lines = 2;

    if (g->state == STATE_MENU) {
        iprintf(" %s\n",           lang_str(STR_START));
        iprintf(" %s\n",           lang_str(STR_ANY_KEY));
        iprintf("\n");
        iprintf(" Y: %s\n",        lang_str(STR_AUTOPILOT));
        iprintf(" L/R: language\n");
        lines += 5;
    } else if (g->state == STATE_WIN) {
        iprintf(" %s\n",  lang_str(STR_WIN));
        iprintf(" %s\n",  lang_str(STR_RESTART));
        iprintf(" %s\n",  lang_str(STR_ANY_KEY));
        lines += 3;
    } else if (g->state == STATE_LOSE) {
        iprintf(" %s\n",  lang_str(STR_LOSE));
        iprintf(" %s\n",  lang_str(STR_RESTART));
        iprintf(" %s\n",  lang_str(STR_ANY_KEY));
        lines += 3;
    } else {
        /* PLAYING / PAUSED */
        iprintf(" %s %d/%d\n", lang_str(STR_SIZE),
                g->snake.length, WIN_LENGTH);
        iprintf(" ");
        draw_progress(g->snake.length, WIN_LENGTH, 20);
        iprintf("\n");
        /* Food legend: ANSI color codes — red=31, green=32, reset=0 */
        iprintf("\n \x1b[31m@\x1b[0m:-5   \x1b[32m@\x1b[0m:+5\n");
        lines += 4;

        if (g->state == STATE_PAUSED) {
            iprintf("\n ** %s **\n", lang_str(STR_PAUSED));
            lines += 2;
        } else {
            iprintf("\n");
            lines += 1;
        }

        iprintf(" %s:%s  [Y]\n", lang_str(STR_AUTOPILOT),
                g->autopilot ? lang_str(STR_ON) : lang_str(STR_OFF));
        lines += 1;
    }

    /* Pad blank lines to anchor language section at the screen bottom */
    while (lines < 22) { iprintf("\n"); lines++; }

    /* Row 22: L/R hint; Row 23: language selector */
    iprintf("   L: prev        R: next\n");
    iprintf(" < %-24s >\n", LANG_META[lang_get_current()].native_name);
}

/* ── Language auto-detection ───────────────────────────────────────── */
/* NDS PersonalData->language: 0=JP 1=EN 2=FR 3=DE 4=IT 5=ES 6=ZH */
Language lang_detect_system(void) {
    switch (PersonalData->language) {
        case 1: return LANG_EN;
        case 2: return LANG_FR;
        case 3: return LANG_DE;
        case 4: return LANG_IT;
        case 5: return LANG_ES;
        default: return LANG_EN;
    }
}
