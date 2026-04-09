#include "render_3ds.h"
#include "../../numberslide-common/sprites/tile_data.h"
#include "../../numberslide-common/anim.h"
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>

/* ── Colours ── */

#define CLR_BG       C2D_Color32(15,  15,  26,  255)
#define CLR_BOT_BG   C2D_Color32(20,  20,  40,  255)
#define CLR_BTN_BG   C2D_Color32(50,  50,  80,  255)
#define CLR_BTN_TXT  C2D_Color32(220, 220, 255, 255)
#define CLR_WHITE    C2D_Color32(255, 255, 255, 255)
#define CLR_GRAY     C2D_Color32(150, 150, 180, 255)
#define CLR_WIN      C2D_Color32( 74, 222, 128, 255)
#define CLR_LOSE     C2D_Color32(248, 113, 113, 255)
#define CLR_ORANGE   C2D_Color32(237, 194,  46, 255)

static u32 rgb24_to_c2d(unsigned int rgb) {
    return C2D_Color32((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 0xFF);
}

/* ── Text helpers ── */

static C2D_TextBuf g_tbuf;
static C2D_Font    g_font_sys;

void render_init_3ds(void) {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    g_tbuf     = C2D_TextBufNew(4096);
    g_font_sys = NULL;
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

static void draw_text_centered(float cx, float cy, float sz, u32 color, const char *str) {
    C2D_Text txt;
    C2D_TextBufClear(g_tbuf);
    C2D_TextFontParse(&txt, g_font_sys, g_tbuf, str);
    C2D_TextOptimize(&txt);
    float w, h;
    C2D_TextGetDimensions(&txt, sz, sz, &w, &h);
    /* Draw at top-left, centered both horizontally and vertically */
    C2D_DrawText(&txt, C2D_WithColor,
                 cx - w / 2.0f, cy - h / 2.0f, 0.5f, sz, sz, color);
}

/* ── Tile rendering helpers ── */

static float cell_fx(int col) { return BOARD_OFFSET_X + col * (TILE_SIZE + TILE_GAP); }
static float cell_fy(int row) { return BOARD_OFFSET_Y + row * (TILE_SIZE + TILE_GAP); }

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

static void draw_tile_3ds(float ox, float oy, float size, int value) {
    u32 bg;
    const TileStyle *s = tile_style_for(value);
    if (s)              bg = rgb24_to_c2d(s->bg);
    else if (value > 0) bg = rgb24_to_c2d(TILE_SUPER_BG);
    else                bg = rgb24_to_c2d(TILE_STYLES[0].bg);

    C2D_DrawRectSolid(ox, oy, 0.5f, size, size, bg);

    if (value > 0) {
        u32 fg;
        if (s) fg = rgb24_to_c2d(s->fg);
        else   fg = rgb24_to_c2d(TILE_SUPER_FG);

        char buf[12];
        snprintf(buf, sizeof(buf), "%d", value);

        float sz = 0.7f;
        int len = (int)strlen(buf);
        if (len >= 4) sz = 0.5f;
        else if (len >= 3) sz = 0.6f;

        /* Scale font proportionally if tile is smaller/larger */
        sz *= size / (float)TILE_SIZE;

        float tcx = ox + size / 2.0f;
        float tcy = oy + size / 2.0f;
        draw_text_centered(tcx, tcy, sz, fg, buf);
    }
}

/* ── Top screen ── */

static void render_top_3ds(const Game *g) {
    int bw = GRID_SIZE * TILE_SIZE + (GRID_SIZE - 1) * TILE_GAP;
    int pad = 6;
    C2D_DrawRectSolid(BOARD_OFFSET_X - pad, BOARD_OFFSET_Y - pad, 0.3f,
                      bw + pad * 2, bw + pad * 2,
                      rgb24_to_c2d(BOARD_BG_COLOR));

    int animating = anim_busy(&g->anim);

    if (!animating) {
        for (int r = 0; r < GRID_SIZE; r++)
            for (int c = 0; c < GRID_SIZE; c++)
                draw_tile_3ds(cell_fx(c), cell_fy(r), TILE_SIZE, g->board.cells[r][c]);
    } else {
        /* Draw empty cell backgrounds */
        for (int r = 0; r < GRID_SIZE; r++)
            for (int c = 0; c < GRID_SIZE; c++)
                draw_tile_3ds(cell_fx(c), cell_fy(r), TILE_SIZE, 0);

        /* Non-animated cells */
        int animated[GRID_SIZE][GRID_SIZE] = {{0}};
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            animated[a->to_row][a->to_col] = 1;
            if (a->type == ANIM_SLIDE)
                animated[a->from_row][a->from_col] = 1;
        }
        for (int r = 0; r < GRID_SIZE; r++)
            for (int c = 0; c < GRID_SIZE; c++)
                if (!animated[r][c] && g->board.cells[r][c] > 0)
                    draw_tile_3ds(cell_fx(c), cell_fy(r), TILE_SIZE, g->board.cells[r][c]);

        /* SLIDE animations */
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            if (a->type != ANIM_SLIDE) continue;
            float t = (float)a->elapsed_ms / a->duration_ms;
            if (t > 1.0f) t = 1.0f;
            float px = lerpf(cell_fx(a->from_col), cell_fx(a->to_col), t);
            float py = lerpf(cell_fy(a->from_row), cell_fy(a->to_row), t);
            draw_tile_3ds(px, py, TILE_SIZE, a->value);
        }

        /* MERGE animations (pop) */
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            if (a->type != ANIM_MERGE) continue;
            float t = (float)a->elapsed_ms / a->duration_ms;
            if (t > 1.0f) t = 1.0f;
            /* Triangle scale: 1.0 → 1.2 → 1.0 */
            float scale;
            if (t < 0.5f)
                scale = 1.0f + 0.4f * t;       /* 1.0 → 1.2 */
            else
                scale = 1.2f - 0.4f * (t - 0.5f); /* 1.2 → 1.0 */
            float sz = TILE_SIZE * scale;
            float px = cell_fx(a->to_col) + (TILE_SIZE - sz) / 2.0f;
            float py = cell_fy(a->to_row) + (TILE_SIZE - sz) / 2.0f;
            draw_tile_3ds(px, py, sz, a->value);
        }

        /* SPAWN animations (scale up) */
        for (int i = 0; i < g->anim.count; i++) {
            const Anim *a = &g->anim.items[i];
            if (a->type != ANIM_SPAWN) continue;
            float t = (float)a->elapsed_ms / a->duration_ms;
            if (t > 1.0f) t = 1.0f;
            float sz = TILE_SIZE * t;
            if (sz < 1.0f) sz = 1.0f;
            float px = cell_fx(a->to_col) + (TILE_SIZE - sz) / 2.0f;
            float py = cell_fy(a->to_row) + (TILE_SIZE - sz) / 2.0f;
            draw_tile_3ds(px, py, sz, a->value);
        }
    }

    /* State overlays */
    if (g->state == STATE_PAUSED || g->state == STATE_WIN ||
        g->state == STATE_GAME_OVER) {
        C2D_DrawRectSolid(0, 0, 0.9f, SCREEN_W, SCREEN_H,
                          C2D_Color32(0, 0, 0, 160));
        u32 tcol = (g->state == STATE_WIN)       ? CLR_WIN  :
                   (g->state == STATE_GAME_OVER)  ? CLR_LOSE : CLR_WHITE;
        const char *title = (g->state == STATE_WIN)       ? lang_str(STR_YOU_WIN)   :
                            (g->state == STATE_GAME_OVER)  ? lang_str(STR_GAME_OVER) :
                            lang_str(STR_PAUSED);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f, 1.0f, tcol, title);
    }
}

/* ── Bottom screen ── */

static void render_bottom_3ds(const Game *g) {
    C2D_DrawRectSolid(0, 0, 0.5f, BOT_SCREEN_W, BOT_SCREEN_H, CLR_BOT_BG);

    float y = 15;
    draw_text(10, y, 0.7f, CLR_ORANGE, lang_str(STR_TITLE)); y += 22;

    char buf[48];
    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_SCORE), g->board.score);
    draw_text(10, y, 0.6f, CLR_WHITE, buf); y += 18;

    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_BEST), g->best_score);
    draw_text(10, y, 0.55f, CLR_GRAY, buf); y += 22;

    switch (g->state) {
    case STATE_MENU:
        draw_text(10, y, 0.55f, CLR_GRAY, lang_str(STR_ANY_KEY));
        break;
    case STATE_PLAYING:
        draw_text(10, y, 0.48f, CLR_GRAY, "D-pad / Touch: slide"); y += 14;
        draw_text(10, y, 0.48f, CLR_GRAY, "START: pause");         y += 14;
        draw_text(10, y, 0.48f, CLR_GRAY, "SELECT: new game");
        break;
    case STATE_PAUSED:
        draw_text(10, y, 0.6f, CLR_WHITE, lang_str(STR_PAUSED)); y += 20;
        draw_text(10, y, 0.48f, CLR_GRAY, "START/A: resume");
        break;
    case STATE_WIN:
        draw_text(10, y, 0.65f, CLR_WIN, lang_str(STR_YOU_WIN)); y += 22;
        snprintf(buf, sizeof(buf), "A: %s", lang_str(STR_CONTINUE));
        draw_text(10, y, 0.5f, CLR_GRAY, buf); y += 16;
        snprintf(buf, sizeof(buf), "B: %s", lang_str(STR_NEW_GAME));
        draw_text(10, y, 0.5f, CLR_GRAY, buf);
        break;
    case STATE_GAME_OVER:
        draw_text(10, y, 0.65f, CLR_LOSE, lang_str(STR_GAME_OVER)); y += 22;
        snprintf(buf, sizeof(buf), "A: %s", lang_str(STR_NEW_GAME));
        draw_text(10, y, 0.5f, CLR_GRAY, buf);
        break;
    }

    draw_text(LANG_BTN_X + 4, LANG_BTN_Y - 14, 0.45f, CLR_GRAY,
              "L: prev   R: next");
    C2D_DrawRectSolid(LANG_BTN_X, LANG_BTN_Y, 0.5f,
                      LANG_BTN_W, LANG_BTN_H, CLR_BTN_BG);
    draw_text(LANG_BTN_X + 8, LANG_BTN_Y + LANG_BTN_H - 7, 0.55f, CLR_BTN_TXT,
              LANG_META[lang_get_current()].native_name);
    draw_text(LANG_BTN_X + LANG_BTN_W - 20, LANG_BTN_Y + LANG_BTN_H - 7,
              0.55f, CLR_BTN_TXT, ">");
}

/* ── Main render ── */

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

/* ── Language auto-detection ── */

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
