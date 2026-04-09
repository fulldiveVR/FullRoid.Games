#include "render_3ds.h"
#include "../../lunarrunner-common/i18n/i18n.h"
#include "../../lunarrunner-common/font.h"
#include <stdio.h>
#include <string.h>

/*
 * 3DS renderer using citro2d.
 * All objects drawn as a few colored rectangles each — NOT per-pixel.
 * citro2d has a ~4096 draw call limit per frame; this renderer stays under ~200.
 */

#define CLR_SKY       C2D_Color32(0,   0,   15,  255)
#define CLR_BOT_BG    C2D_Color32(20,  20,  40,  255)
#define CLR_WHITE     C2D_Color32(255, 255, 255, 255)
#define CLR_GRAY      C2D_Color32(150, 150, 180, 255)
#define CLR_ORANGE    C2D_Color32(237, 194, 46,  255)
#define CLR_LOSE      C2D_Color32(248, 113, 113, 255)
#define CLR_BAR_BG    C2D_Color32(50,  50,  80,  255)
#define CLR_BAR_FG    C2D_Color32(100, 200, 100, 255)
#define CLR_DIM       C2D_Color32(0,   0,   0,   160)

/* Game object colors */
#define CLR_GROUND    C2D_Color32(40,  40,  40,  255)
#define CLR_GROUND_D  C2D_Color32(60,  60,  60,  255)
#define CLR_MTN       C2D_Color32(30,  30,  30,  255)
#define CLR_ROVER     C2D_Color32(170, 170, 170, 255)
#define CLR_ROVER_WIN C2D_Color32(120, 120, 130, 255)
#define CLR_ROVER_LT  C2D_Color32(0,   136, 255, 255)
#define CLR_CRATER    C2D_Color32(15,  15,  15,  255)
#define CLR_BOULDER   C2D_Color32(120, 120, 120, 255)
#define CLR_BOULDER_H C2D_Color32(160, 160, 160, 255)
#define CLR_ANTENNA   C2D_Color32(150, 150, 150, 255)
#define CLR_ANTENNA_P C2D_Color32(100, 100, 100, 255)
#define CLR_FLOWER    C2D_Color32(60,  200, 60,  255)
#define CLR_FLOWER_D  C2D_Color32(30,  130, 30,  255)
#define CLR_CRYSTAL   C2D_Color32(255, 210, 0,   255)
#define CLR_STARDUST  C2D_Color32(255, 255, 180, 255)
#define CLR_SHIELD_C  C2D_Color32(0,   130, 255, 255)
#define CLR_SHIELD_G  C2D_Color32(0,   130, 255, 60)

void render_init_3ds(void) {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
}

void render_exit_3ds(void) {
    C2D_Fini();
    C3D_Fini();
}

/* --- Bitmap font text via run-based rects (efficient draw calls) --- */

typedef struct {
    u32   color;
    float z;
    float scale;
} TextCtx;

static void text_run_cb(int x, int y, int w, void *ctx_ptr) {
    TextCtx *tc = (TextCtx *)ctx_ptr;
    C2D_DrawRectSolid(x * tc->scale, y * tc->scale, tc->z,
                      w * tc->scale, tc->scale, tc->color);
}

static void draw_text(float x, float y, float scale, u32 color, const char *str) {
    TextCtx tc = { color, 0.5f, scale };
    font_draw_string_cb((int)(x / scale), (int)(y / scale), str, text_run_cb, &tc);
}

static void draw_text_centered(float cx, float cy, float scale, u32 color, const char *str) {
    int pw = font_string_width(str);
    int ph = FONT_H;
    float w = pw * scale;
    float h = ph * scale;
    draw_text(cx - w / 2.0f, cy - h / 2.0f, scale, color, str);
}

/* ═══════════════════════════════════════════════════════════
 * Background — minimal draw calls
 * ═══════════════════════════════════════════════════════════ */

static void draw_stars(int offset) {
    int shift = offset / 10;
    for (int i = 0; i < 25; i++) {
        int base_x = (i * 137 + 43) % SCREEN_W;
        int sx = base_x - (shift % SCREEN_W);
        if (sx < 0) sx += SCREEN_W;
        float sy = (float)((i * 89 + 17) % (GROUND_Y - 20));
        u32 c = (i % 3 == 0) ? CLR_WHITE : CLR_GRAY;
        C2D_DrawRectSolid((float)sx, sy, 0.02f, 2, 2, c);
    }
}

#define MTN3_PERIOD 300

static void draw_mountains(int offset) {
    float base = GROUND_Y;
    int shift = (offset / 4) % MTN3_PERIOD;

    for (int tile = -MTN3_PERIOD; tile < SCREEN_W + MTN3_PERIOD; tile += MTN3_PERIOD) {
        float x0 = (float)(tile - shift);
        /* Peak 1 — tall */
        C2D_DrawRectSolid(x0 + 30, base - 24, 0.04f, 40, 24, CLR_MTN);
        C2D_DrawRectSolid(x0 + 15, base - 10, 0.04f, 70, 10, CLR_MTN);
        /* Peak 2 — short */
        C2D_DrawRectSolid(x0 + 160, base - 14, 0.04f, 30, 14, CLR_MTN);
        C2D_DrawRectSolid(x0 + 145, base - 6,  0.04f, 60,  6, CLR_MTN);
    }
}

static void draw_ground(int offset) {
    /* 1 rect for ground + ~7 for detail = 8 calls */
    C2D_DrawRectSolid(0, GROUND_Y, 0.1f, SCREEN_W, SCREEN_H - GROUND_Y, CLR_GROUND);

    int goff = offset % 20;
    for (int x = -goff; x < SCREEN_W; x += 20)
        C2D_DrawRectSolid(x, GROUND_Y + 2, 0.15f, 6, 2, CLR_GROUND_D);
}

/* ═══════════════════════════════════════════════════════════
 * Game objects — 1-3 rects per object
 * ═══════════════════════════════════════════════════════════ */

static void draw_rover(const Game *g) {
    int ry = FP_TO_INT(g->rover.y_fp);
    int rh = rover_height(&g->rover);
    float rx = g->rover.x + g->fx.screen_shake_x;
    float ty = ry - rh + g->fx.screen_shake_y;

    /* Body — 1 rect */
    u32 body_clr = g->rover.has_shield ? CLR_SHIELD_C : CLR_ROVER;
    C2D_DrawRectSolid(rx, ty, 0.6f, ROVER_W, rh - 4, body_clr);

    /* Wheels — 1 rect below body */
    if (g->rover.action != ROVER_JUMP && g->rover.action != ROVER_JUMP_DUCK)
        C2D_DrawRectSolid(rx + 1, ry - 4, 0.58f, ROVER_W - 2, 4, CLR_ROVER_WIN);

    /* Headlight — 1 small rect */
    C2D_DrawRectSolid(rx + ROVER_W - 4, ty + 2, 0.65f, 3, 3, CLR_ROVER_LT);

    /* Antenna — 1 line */
    C2D_DrawRectSolid(rx + 8, ty - 6, 0.65f, 1, 6, CLR_GRAY);
    C2D_DrawRectSolid(rx + 7, ty - 7, 0.65f, 3, 2, CLR_WHITE);

    /* Shield glow */
    if (g->rover.has_shield)
        C2D_DrawRectSolid(rx - 2, ty - 2, 0.55f, ROVER_W + 4, rh + 4, CLR_SHIELD_G);
}

static void draw_obstacle(const Obstacle *o) {
    float ox = (float)FP_TO_INT(o->x_fp);

    switch (o->type) {
    case OBS_CRATER:
        /* Dark pit — 2 rects (rim + hole) */
        C2D_DrawRectSolid(ox - 2, o->y - 1, 0.38f, o->w + 4, o->h + 2, CLR_GROUND_D);
        C2D_DrawRectSolid(ox, o->y, 0.4f, o->w, o->h, CLR_CRATER);
        break;

    case OBS_BOULDER:
        /* Round rock — 3 rects (base + highlight + shadow) */
        C2D_DrawRectSolid(ox + 1, o->y + 2, 0.39f, o->w - 2, o->h - 2, CLR_BOULDER);
        C2D_DrawRectSolid(ox, o->y, 0.4f, o->w, o->h - 4, CLR_BOULDER_H);
        C2D_DrawRectSolid(ox + 2, o->y + o->h - 4, 0.41f, o->w - 4, 3, CLR_MTN);
        break;

    case OBS_ANTENNA:
        /* Metal bar on posts — 3 rects */
        C2D_DrawRectSolid(ox, o->y + 1, 0.4f, o->w, o->h - 2, CLR_ANTENNA);
        C2D_DrawRectSolid(ox + 1, o->y, 0.41f, 2, o->h + 4, CLR_ANTENNA_P);
        C2D_DrawRectSolid(ox + o->w - 3, o->y, 0.41f, 2, o->h + 4, CLR_ANTENNA_P);
        break;

    case OBS_ALIEN_FLOWER:
        /* Flower head + stem — 3 rects */
        C2D_DrawRectSolid(ox + o->w / 2 - 1, o->y + o->h / 2, 0.39f,
                          2, o->h / 2 + 4, CLR_FLOWER_D);
        C2D_DrawRectSolid(ox, o->y, 0.4f, o->w, o->h / 2, CLR_FLOWER);
        C2D_DrawRectSolid(ox + 2, o->y + 2, 0.41f, 3, 2, CLR_WHITE);
        break;
    }
}

/* ═══════════════════════════════════════════════════════════
 * Top screen
 * ═══════════════════════════════════════════════════════════ */

static void render_top_3ds(const Game *g) {
    draw_stars(g->state == STATE_MENU ? 0 : g->world.bg_offset_far);
    draw_mountains(g->state == STATE_MENU ? 0 : g->world.bg_offset_near);
    draw_ground(g->state == STATE_MENU ? 0 : g->world.bg_offset_ground);

    if (g->state == STATE_MENU) {
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f - 40, 4.0f,
                           CLR_ORANGE, lang_str(STR_TITLE));
        C2D_DrawRectSolid(SCREEN_W / 2.0f - 16, SCREEN_H / 2.0f + 5,
                          0.5f, 32, 20, CLR_ROVER);
        C2D_DrawRectSolid(SCREEN_W / 2.0f + 12, SCREEN_H / 2.0f + 7,
                          0.55f, 3, 3, CLR_ROVER_LT);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 40, 2.5f,
                           CLR_GRAY, lang_str(STR_PRESS_A_START));
        return;
    }

    /* Obstacles — max 8 × 3 rects = 24 calls */
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!g->obstacles[i].active) continue;
        draw_obstacle(&g->obstacles[i]);
    }

    /* Collectibles — max 16 × 2 rects = 32 calls */
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        const Collectible *c = &g->collectibles[i];
        if (!c->active) continue;
        float cx = (float)FP_TO_INT(c->x_fp);
        switch (c->type) {
        case COLLECT_CRYSTAL:
            C2D_DrawRectSolid(cx + 1, c->y, 0.5f, 6, 8, CLR_CRYSTAL);
            C2D_DrawRectSolid(cx + 2, c->y + 1, 0.51f, 2, 2, CLR_WHITE);
            break;
        case COLLECT_STARDUST:
            C2D_DrawRectSolid(cx, c->y, 0.5f, 8, 8, CLR_STARDUST);
            C2D_DrawRectSolid(cx + 2, c->y + 2, 0.51f, 4, 4, CLR_WHITE);
            break;
        case COLLECT_SHIELD:
            C2D_DrawRectSolid(cx, c->y, 0.5f, 8, 8, CLR_SHIELD_C);
            C2D_DrawRectSolid(cx + 2, c->y + 2, 0.51f, 4, 4,
                              C2D_Color32(100, 200, 255, 255));
            break;
        }
    }

    /* Rover — ~6 rects */
    draw_rover(g);

    /* Meteor warning */
    if (g->meteor.active && g->meteor.warning_timer > 0) {
        if ((g->meteor.warning_timer / 80) & 1) {
            C2D_DrawRectSolid(0, 0, 0.8f, SCREEN_W, 3, CLR_ORANGE);
            C2D_DrawRectSolid(0, GROUND_Y - 2, 0.8f, SCREEN_W, 3, CLR_ORANGE);
        }
    }

    /* Meteor rain — 12 rects */
    if (g->meteor.active && g->meteor.rain_timer > 0) {
        for (int i = 0; i < 12; i++) {
            float mx = (float)((i * 73 + g->meteor.rain_timer / 2) % SCREEN_W);
            float my = (float)((i * 41 + g->meteor.rain_timer * 3) % (GROUND_Y - 10));
            C2D_DrawRectSolid(mx, my, 0.7f, 2, 6, CLR_ORANGE);
        }
    }

    /* Particles — max 16 rects */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &g->fx.particles[i];
        if (p->life <= 0) continue;
        C2D_DrawRectSolid(p->x, p->y, 0.9f, 3, 3, CLR_CRYSTAL);
    }

    /* Score */
    if (g->state == STATE_PLAYING) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", g->score);
        draw_text(SCREEN_W - 100, 4, 2.0f, CLR_WHITE, buf);
    }

    /* Overlays */
    if (g->state == STATE_GAME_OVER) {
        C2D_DrawRectSolid(0, 0, 0.95f, SCREEN_W, SCREEN_H, CLR_DIM);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f - 30, 3.5f,
                           CLR_LOSE, lang_str(STR_GAME_OVER));
        char buf2[48];
        snprintf(buf2, sizeof(buf2), "%s: %d", lang_str(STR_SCORE), g->score);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 10, 2.5f, CLR_WHITE, buf2);
        snprintf(buf2, sizeof(buf2), "%s: %d", lang_str(STR_BEST), g->best_score);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 35, 2.0f, CLR_ORANGE, buf2);
    }

    if (g->state == STATE_PAUSED) {
        C2D_DrawRectSolid(0, 0, 0.95f, SCREEN_W, SCREEN_H, CLR_DIM);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f, 3.5f,
                           CLR_WHITE, lang_str(STR_PAUSED));
    }
}

/* ═══════════════════════════════════════════════════════════
 * Bottom screen HUD
 * ═══════════════════════════════════════════════════════════ */

static void render_bottom_3ds(const Game *g) {
    C2D_DrawRectSolid(0, 0, 0.5f, BOT_SCREEN_W, BOT_SCREEN_H, CLR_BOT_BG);

    float y = 8;
    draw_text(8, y, 2.5f, CLR_ORANGE, lang_str(STR_TITLE)); y += 22;

    char buf[64];
    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_SCORE), g->score);
    draw_text(8, y, 2.0f, CLR_WHITE, buf); y += 18;

    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_BEST), g->best_score);
    draw_text(8, y, 1.8f, CLR_GRAY, buf); y += 16;

    snprintf(buf, sizeof(buf), "%s: %d", lang_str(STR_DISTANCE), g->world.distance);
    draw_text(8, y, 1.8f, CLR_GRAY, buf); y += 20;

    /* Bonus bar */
    float bar_x = 8, bar_w = BOT_SCREEN_W - 16, bar_h = 12;
    C2D_DrawRectSolid(bar_x, y, 0.5f, bar_w, bar_h, CLR_BAR_BG);
    float fill_w = bar_w * g->bonus.bar / BONUS_BAR_MAX;
    if (fill_w > 0)
        C2D_DrawRectSolid(bar_x, y, 0.6f, fill_w, bar_h, CLR_BAR_FG);
    y += bar_h + 6;

    if (g->bonus.stored != BONUS_NONE) {
        const char *name = "???";
        if (g->bonus.stored == BONUS_SOLAR)  name = lang_str(STR_SOLAR);
        if (g->bonus.stored == BONUS_TURBO)  name = lang_str(STR_TURBO);
        if (g->bonus.stored == BONUS_MAGNET) name = lang_str(STR_MAGNET);
        snprintf(buf, sizeof(buf), "%s: %s [X/Y]", lang_str(STR_READY), name);
        draw_text(8, y, 1.8f, CLR_ORANGE, buf);
    }
    y += 16;

    if (g->bonus.solar_timer > 0) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_SOLAR), lang_str(STR_ACTIVE));
        draw_text(8, y, 1.5f, CLR_CRYSTAL, buf); y += 14;
    }
    if (g->bonus.turbo_timer > 0) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_TURBO), lang_str(STR_ACTIVE));
        draw_text(8, y, 1.5f, CLR_SHIELD_C, buf); y += 14;
    }
    if (g->bonus.magnet_timer > 0) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_MAGNET), lang_str(STR_ACTIVE));
        draw_text(8, y, 1.5f, CLR_ORANGE, buf); y += 14;
    }
    if (g->rover.has_shield) {
        snprintf(buf, sizeof(buf), "%s %s", lang_str(STR_SHIELD), lang_str(STR_ACTIVE));
        draw_text(8, y, 1.5f, CLR_SHIELD_C, buf);
    }

    /* Language indicator */
    float lang_y = BOT_SCREEN_H - 42;
    snprintf(buf, sizeof(buf), "L/R: %s", LANG_META[lang_get_current()].native_name);
    draw_text(8, lang_y, 1.5f, CLR_GRAY, buf);

    /* State hints — below language */
    float by = BOT_SCREEN_H - 22;
    switch (g->state) {
    case STATE_MENU:
        draw_text(8, by, 2.0f, CLR_GRAY, lang_str(STR_PRESS_A_START));
        break;
    case STATE_PLAYING:
        snprintf(buf, sizeof(buf), "A/B:%s Down:%s X/Y:%s",
                 lang_str(STR_JUMP), lang_str(STR_DUCK), lang_str(STR_BONUS));
        draw_text(8, by, 1.5f, CLR_GRAY, buf);
        break;
    case STATE_PAUSED:
        snprintf(buf, sizeof(buf), "%s  %s", lang_str(STR_RESUME), lang_str(STR_QUIT));
        draw_text(8, by, 2.0f, CLR_GRAY, buf);
        break;
    case STATE_GAME_OVER:
        snprintf(buf, sizeof(buf), "%s  %s", lang_str(STR_RETRY), lang_str(STR_MENU));
        draw_text(8, by, 2.0f, CLR_GRAY, buf);
        break;
    }
}

/* ═══════════════════════════════════════════════════════════ */

void render_frame_3ds(const Game *g,
                      C3D_RenderTarget *top,
                      C3D_RenderTarget *bot) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    C2D_TargetClear(top, CLR_SKY);
    C2D_SceneBegin(top);
    render_top_3ds(g);

    C2D_TargetClear(bot, CLR_BOT_BG);
    C2D_SceneBegin(bot);
    render_bottom_3ds(g);

    C3D_FrameEnd(0);
}
