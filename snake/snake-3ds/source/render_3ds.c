#include "render_3ds.h"
#include "../../snake-common/sprites/sprite_data.h"
#include <3ds.h>
#include <citro2d.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ── Colours ────────────────────────────────────────────────────────── */

/* Compile-time constant equivalent of C2D_Color32 (inline fn cannot init statics) */
#define C2D_COLOR32(r,g,b,a) \
    ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16) | ((u32)(a) << 24))

#define CLR_BG      C2D_Color32(15,  15,  26, 255)
#define CLR_BOT_BG  C2D_Color32(20,  20,  40, 255)
#define CLR_BTN_BG  C2D_Color32(50,  50,  80, 255)
#define CLR_BTN_TXT C2D_Color32(220, 220, 255, 255)
#define CLR_WHITE   C2D_Color32(255, 255, 255, 255)
#define CLR_GRAY    C2D_Color32(150, 150, 180, 255)
#define CLR_GREEN   C2D_Color32( 74, 222, 128, 255)
#define CLR_WIN     C2D_Color32( 74, 222, 128, 255)
#define CLR_LOSE    C2D_Color32(248, 113, 113, 255)

/* Snake body colour — 5-stop gradient indexed by segment position */
static u32 snake_color(int idx, int length) {
    static const u32 stops[5] = {
        C2D_COLOR32( 74, 222, 128, 255),
        C2D_COLOR32( 96, 165, 250, 255),
        C2D_COLOR32(139,  92, 246, 255),
        C2D_COLOR32(248, 113, 113, 255),
        C2D_COLOR32(251, 191,  36, 255),
    };
    if (length <= 1) return stops[0];
    int t = idx * 4 / (length - 1);
    if (t >= 4) t = 4;
    /* Nearest-stop — sufficient for handheld hardware */
    return stops[t];
}

/* ── 8×8 sprite via C2D_DrawRectSolid ───────────────────────────── */

static const u32 CPAL_RED[5] = {
    0, /* transparent */
    C2D_COLOR32(0xFF, 0x99, 0x99, 0xFF),
    C2D_COLOR32(0xDD, 0x22, 0x22, 0xFF),
    C2D_COLOR32(0x99, 0x11, 0x00, 0xFF),
    C2D_COLOR32(0x44, 0x00, 0x00, 0xFF),
};
static const u32 CPAL_BLUE[5] = {
    0,
    C2D_COLOR32(0x99, 0xCC, 0xFF, 0xFF),
    C2D_COLOR32(0x22, 0x55, 0xEE, 0xFF),
    C2D_COLOR32(0x00, 0x33, 0xAA, 0xFF),
    C2D_COLOR32(0x00, 0x11, 0x55, 0xFF),
};
static const u32 CPAL_HEAD[5] = {
    0,
    C2D_COLOR32(0xAA, 0xFF, 0x99, 0xFF),
    C2D_COLOR32(0x22, 0xBB, 0x44, 0xFF),
    C2D_COLOR32(0x00, 0x77, 0x22, 0xFF),
    C2D_COLOR32(0x00, 0x33, 0x11, 0xFF),
};

static void draw_sprite8(float ox, float oy,
                         const uint8_t spr[8][8],
                         const u32 pal[5]) {
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            uint8_t idx = spr[dy][dx];
            if (idx == 0) continue;
            C2D_DrawRectSolid(ox + dx, oy + dy, 0.5f, 1.0f, 1.0f, pal[idx]);
        }
    }
}

/* ── Text helpers ────────────────────────────────────────────────── */

static C2D_TextBuf g_tbuf;
static C2D_Font    g_font_sys;

void render_init_3ds(void) {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    g_tbuf     = C2D_TextBufNew(4096);
    g_font_sys = NULL; /* use built-in system font */
}

void render_exit_3ds(void) {
    C2D_TextBufDelete(g_tbuf);
    C2D_Fini();
    C3D_Fini();
}

static void draw_text(float x, float y, float sz, u32 color, const char *str) {
    C2D_Text txt;
    C2D_TextBufClear(g_tbuf);
    C2D_TextFontParse(&txt, g_font_sys, g_tbuf, str);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_WithColor | C2D_AtBaseline,
                 x, y, 0.5f, sz, sz, color);
}

/* ── Progress bar ────────────────────────────────────────────────── */

static void draw_progress_bar(float x, float y, float w, float h,
                               int val, int max) {
    C2D_DrawRectSolid(x, y, 0.5f, w, h,
                      C2D_Color32(40, 40, 60, 255));
    float fill = (max > 0) ? ((float)val / max * w) : 0;
    if (fill > w) fill = w;
    C2D_DrawRectSolid(x, y, 0.5f, fill, h, CLR_GREEN);
}

/* ── Top screen ──────────────────────────────────────────────────── */

void render_top_3ds(const Game *g) {
    int ox = (SCREEN_W - FIELD_WIDTH  * CELL_SIZE) / 2;
    int oy = (SCREEN_H - FIELD_HEIGHT * CELL_SIZE) / 2;

    /* Food */
    for (int i = 0; i < g->food.count; i++) {
        float fx = ox + g->food.items[i].x * CELL_SIZE;
        float fy = oy + g->food.items[i].y * CELL_SIZE;
        const u32 *pal = (g->food.items[i].type == FOOD_RED)
                         ? CPAL_RED : CPAL_BLUE;
        draw_sprite8(fx, fy, SPR_FOOD_8x8, pal);
    }

    /* Snake — tail to head */
    for (int i = g->snake.length - 1; i >= 0; i--) {
        float sx = ox + g->snake.segments[i].x * CELL_SIZE;
        float sy = oy + g->snake.segments[i].y * CELL_SIZE;
        float r  = CELL_SIZE / 2.0f - 0.5f;
        float cx = sx + CELL_SIZE / 2.0f;
        float cy = sy + CELL_SIZE / 2.0f;

        if (i == 0) {
            const uint8_t (*hspr)[8] = HEAD_SPRITES_3DS[g->snake.direction];
            draw_sprite8(sx, sy, hspr, CPAL_HEAD);
        } else {
            C2D_DrawCircleSolid(cx, cy, 0.5f, r, snake_color(i, g->snake.length));
        }
    }

    /* Overlay for menu / end / pause states */
    if (g->state == STATE_MENU || g->state == STATE_WIN ||
        g->state == STATE_LOSE || g->state == STATE_PAUSED) {
        C2D_DrawRectSolid(0, 0, 0.9f, SCREEN_W, SCREEN_H,
                          C2D_Color32(0, 0, 0, 180));
        u32 title_col = (g->state == STATE_WIN)  ? CLR_WIN  :
                        (g->state == STATE_LOSE) ? CLR_LOSE :
                        CLR_WHITE;
        const char *title = (g->state == STATE_PAUSED) ? lang_str(STR_PAUSED)  :
                            (g->state == STATE_WIN)    ? lang_str(STR_WIN)     :
                            (g->state == STATE_LOSE)   ? lang_str(STR_LOSE)    :
                                                         lang_str(STR_TITLE);
        draw_text(SCREEN_W / 2.0f - 60, SCREEN_H / 2.0f - 20,
                  0.9f, title_col, title);

        if (g->state != STATE_PAUSED) {
            const char *sub = (g->state == STATE_MENU)
                              ? lang_str(STR_START) : lang_str(STR_RESTART);
            draw_text(SCREEN_W / 2.0f - 40, SCREEN_H / 2.0f + 10,
                      0.6f, CLR_GRAY, sub);
            draw_text(SCREEN_W / 2.0f - 60, SCREEN_H / 2.0f + 28,
                      0.45f, CLR_GRAY, lang_str(STR_ANY_KEY));
        }
    }
}

/* ── Bottom screen ───────────────────────────────────────────────── */

void render_bottom_3ds(const Game *g) {
    C2D_DrawRectSolid(0, 0, 0.5f, BOT_SCREEN_W, BOT_SCREEN_H, CLR_BOT_BG);

    float y = 13;
    draw_text(8, y, 0.65f, CLR_WHITE, lang_str(STR_TITLE)); y += 20;

    if (g->state == STATE_MENU) {
        draw_text(8, y, 0.55f, CLR_GRAY, lang_str(STR_START)); y += 16;
        draw_text(8, y, 0.48f, CLR_GRAY, lang_str(STR_ANY_KEY)); y += 14;
        y += 6;
        char ap_hint[48];
        snprintf(ap_hint, sizeof(ap_hint), "Y: %s", lang_str(STR_AUTOPILOT));
        draw_text(8, y, 0.48f, CLR_GRAY, ap_hint);

    } else if (g->state == STATE_WIN) {
        draw_text(8, y, 0.65f, CLR_WIN,  lang_str(STR_WIN));  y += 20;
        draw_text(8, y, 0.55f, CLR_GRAY, lang_str(STR_RESTART)); y += 16;
        draw_text(8, y, 0.48f, CLR_GRAY, lang_str(STR_ANY_KEY));

    } else if (g->state == STATE_LOSE) {
        draw_text(8, y, 0.65f, CLR_LOSE, lang_str(STR_LOSE)); y += 20;
        draw_text(8, y, 0.55f, CLR_GRAY, lang_str(STR_RESTART)); y += 16;
        draw_text(8, y, 0.48f, CLR_GRAY, lang_str(STR_ANY_KEY));

    } else {
        /* PLAYING / PAUSED */
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %d / %d",
                 lang_str(STR_SIZE), g->snake.length, WIN_LENGTH);
        draw_text(8, y, 0.55f, CLR_GRAY, buf); y += 16;

        draw_progress_bar(8, y, BOT_SCREEN_W - 16, 8,
                          g->snake.length, WIN_LENGTH); y += 22;

        /* Food legend: draw actual food sprites with label */
        draw_sprite8(8,  y, SPR_FOOD_8x8, CPAL_RED);
        draw_text(20, y + 7, 0.45f, CLR_LOSE,  "-5");
        draw_sprite8(55, y, SPR_FOOD_8x8, CPAL_BLUE);
        draw_text(67, y + 7, 0.45f, CLR_GREEN, "+5");
        y += 14;

        if (g->state == STATE_PAUSED) {
            draw_text(8, y, 0.6f, CLR_WHITE, lang_str(STR_PAUSED)); y += 18;
        } else {
            y += 18;
        }

        char ap[56];
        snprintf(ap, sizeof(ap), "Y: %s  [%s]",
                 lang_str(STR_AUTOPILOT),
                 g->autopilot ? lang_str(STR_ON) : lang_str(STR_OFF));
        draw_text(8, y, 0.5f, CLR_GRAY, ap);
    }

    /* Language section — fixed bottom zone */
    /* L/R hint above the button */
    draw_text(LANG_BTN_X + 4, LANG_BTN_Y - 16, 0.45f, CLR_GRAY,
              "L: prev   R: next");

    /* Button background */
    C2D_DrawRectSolid(LANG_BTN_X, LANG_BTN_Y, 0.5f,
                      LANG_BTN_W, LANG_BTN_H, CLR_BTN_BG);
    /* Language name — vertically centered in button */
    draw_text(LANG_BTN_X + 8, LANG_BTN_Y + LANG_BTN_H - 7, 0.55f, CLR_BTN_TXT,
              LANG_META[lang_get_current()].native_name);
    /* Right arrow */
    draw_text(LANG_BTN_X + LANG_BTN_W - 20, LANG_BTN_Y + LANG_BTN_H - 7,
              0.55f, CLR_BTN_TXT, ">");
}

/* ── Main render function ────────────────────────────────────────── */

void render_frame_3ds(const Game *g,
                      C3D_RenderTarget *top,
                      C3D_RenderTarget *bot) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    C2D_TargetClear(top, CLR_BG);
    C2D_SceneBegin(top);
    render_top_3ds(g);

    C2D_TargetClear(bot, CLR_BOT_BG);
    C2D_SceneBegin(bot);
    render_bottom_3ds(g);

    C3D_FrameEnd(0);
}

/* ── Language auto-detection ─────────────────────────────────────── */

Language lang_detect_system(void) {
    u8 sys = 0;
    cfguInit();
    CFGU_GetSystemLanguage(&sys);
    cfguExit();
    switch (sys) {
        case CFG_LANGUAGE_EN: return LANG_EN;
        case CFG_LANGUAGE_FR: return LANG_FR;
        case CFG_LANGUAGE_DE: return LANG_DE;
        case CFG_LANGUAGE_IT: return LANG_IT;
        case CFG_LANGUAGE_ES: return LANG_ES;
        case CFG_LANGUAGE_PT: return LANG_PT_PT;
        case CFG_LANGUAGE_RU: return LANG_RU;
        case CFG_LANGUAGE_NL: return LANG_NL;
        default:              return LANG_EN;
    }
}
