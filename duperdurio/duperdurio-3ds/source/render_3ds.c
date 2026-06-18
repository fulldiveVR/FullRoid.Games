#include "render_3ds.h"
#include "../../duperdurio-common/font.h"
#include "../../duperdurio-common/config.h"
#include "../../duperdurio-common/level.h"
#include "../../duperdurio-common/sprites/sprite_atlas.h"
#include <stdio.h>
#include <string.h>

/*
 * 3DS renderer using citro2d.
 *
 * Entities (Durio, enemies, collectible nuts) are drawn from an AI-generated
 * sprite atlas (romfs:/gfx/sprites.t3x, built by tools/gen_sprites.py + tex3ds).
 * Tiles and the parallax background stay procedural (colored rectangles): AI
 * can't make seamless edge-to-edge block fills, and the hand-built tiles tile
 * perfectly and match the theme.
 *
 * If the atlas fails to load (g_sheet == NULL) every entity falls back to its
 * original rectangle drawing, so the build is never visually broken.
 *
 * citro2d cap: ~4096 draw calls/frame; this stays well under 400.
 */

/* Compiled-in sprite atlas; NULL until loaded (or if loading fails). */
static C2D_SpriteSheet g_sheet = NULL;

/* DEBUG: on-screen build tag + entity hitbox outlines. Set DD_DEBUG to 1 to show. */
#define DD_DEBUG    0
#define DD_VERSION  "B17"

/* Sprite sizing: each sprite is fit by its LARGER dimension into a
   (hitbox * SCALE) square, so every entity ends up the same bounding size
   regardless of art aspect ratio (wide crab vs tall Durio) and stays close to
   its 24px hitbox. Player and enemies share one scale for consistency. */
#define DURIO_SPRITE_SCALE 1.25f
#define ENEMY_SPRITE_SCALE 1.25f
#define NUT_SPRITE_SCALE   0.90f

/*
 * Draw an atlas sub-image fitted into a logical hitbox.
 *   box_*      : the entity's logical box in screen px (where rectangles drew).
 *   scale_mul  : HD overdraw — >1 makes the sprite bigger than its hitbox
 *                (kept centered horizontally, bottom-aligned so feet stay put).
 *   sx_mul/sy_mul : per-frame squash/stretch for procedural animation.
 *   y_off      : vertical bob.
 *   flip       : mirror horizontally (sprites are authored facing one way).
 */
static void draw_image_fit(int idx, float box_x, float box_y, float box_w, float box_h,
                           float z, int flip, float scale_mul,
                           float sx_mul, float sy_mul, float y_off) {
    if (!g_sheet) return;
    C2D_Image img = C2D_SpriteSheetGetImage(g_sheet, idx);
    float iw = (float)img.subtex->width;
    float ih = (float)img.subtex->height;
    if (iw < 1.0f || ih < 1.0f) return;

    float md   = (iw > ih) ? iw : ih;        /* fit by the larger dimension */
    float base = (box_h / md) * scale_mul;   /* uniform bounding size across art */
    float sx = base * sx_mul;
    float sy = base * sy_mul;
    float dw = iw * sx;
    float dh = ih * sy;
    float px = box_x + box_w * 0.5f - dw * 0.5f;   /* center on box X     */
    float py = box_y + box_h - dh + y_off;         /* bottom-align to box  */

    /* citro2d mirrors in place when scaleX is negative (draws right from px with
       flipped texcoords), so px stays the same for both facings — adding dw here
       shifted the mirrored sprite right by its full width. */
    float draw_sx = flip ? -sx : sx;
    C2D_DrawImageAt(img, px, py, z, NULL, draw_sx, sy);
}

/* ── Color palette ── */
#define CLR_SKY         C2D_Color32(8,   6,   28,  255)  /* deep space */
#define CLR_SKY_BOTTOM  C2D_Color32(30,  18,  60,  255)  /* horizon glow */
#define CLR_STAR_FAR    C2D_Color32(140, 145, 190, 255)  /* dim distant stars */
#define CLR_STAR_NEAR   C2D_Color32(210, 215, 255, 255)  /* bright near stars */
#define CLR_PLANET_HI   C2D_Color32(80,  200, 175, 255)  /* teal lit side */
#define CLR_PLANET_MID  C2D_Color32(50,  160, 140, 255)  /* teal mid */
#define CLR_PLANET_DARK C2D_Color32(25,  85,  75,  255)  /* teal shadow side */
#define CLR_BOT_BG      C2D_Color32(20,  20,  40,  255)
#define CLR_WHITE       C2D_Color32(255, 255, 255, 255)
#define CLR_BLACK       C2D_Color32(0,   0,   0,   255)
#define CLR_GRAY        C2D_Color32(150, 150, 150, 255)
#define CLR_DIM         C2D_Color32(0,   0,   0,   180)
#define CLR_YELLOW      C2D_Color32(255, 220, 0,   255)
#define CLR_ORANGE      C2D_Color32(240, 120, 0,   255)
#define CLR_RED         C2D_Color32(200, 30,  30,  255)
#define CLR_SKIN        C2D_Color32(252, 188, 116, 255)
#define CLR_BLUE        C2D_Color32(40,  60,  200, 255)
#define CLR_BROWN       C2D_Color32(120, 60,  20,  255)
/* Tiles — alien/sci-fi look */
#define CLR_ROCK_TOP    C2D_Color32(90,  115, 135, 255)  /* blue-gray surface */
#define CLR_ROCK_BODY   C2D_Color32(55,  70,  85,  255)  /* dark rock */
#define CLR_ROCK_CRACK  C2D_Color32(35,  45,  55,  255)  /* crack/seam */
#define CLR_METAL_BASE  C2D_Color32(120, 132, 145, 255)  /* steel panel */
#define CLR_METAL_LIGHT C2D_Color32(185, 200, 215, 255)  /* bevel highlight */
#define CLR_METAL_DARK  C2D_Color32(60,  70,  80,  255)  /* bevel shadow */
#define CLR_QBLOCK_BG   C2D_Color32(220, 160, 0,   255)
#define CLR_QBLOCK_USED C2D_Color32(140, 100, 60,  255)
#define CLR_TOTEM_BODY  C2D_Color32(50,  44,  62,  255)  /* dark basalt */
#define CLR_TOTEM_EDGE  C2D_Color32(78,  70,  94,  255)  /* lighter edge */
#define CLR_TOTEM_GLYPH C2D_Color32(220, 100, 20,  255)  /* orange rune */
#define CLR_PLATFORM      C2D_Color32(65,  55,  80,  255)  /* dark purple-gray platform */
#define CLR_PLATFORM_D    C2D_Color32(95,  82,  115, 255)  /* lighter top edge */
/* Durio — alien space explorer */
#define CLR_DURIO_HEAD    C2D_Color32(40,  210, 200, 255)  /* cyan helmet */
#define CLR_DURIO_VISOR   C2D_Color32(15,  25,  55,  255)  /* dark visor */
#define CLR_DURIO_BODY    C2D_Color32(110, 40,  170, 255)  /* purple suit */
#define CLR_DURIO_BOOTS   C2D_Color32(230, 110, 20,  255)  /* orange boots */
#define CLR_DURIO_ANTENNA C2D_Color32(250, 220, 0,   255)  /* yellow antenna */
/* Enemies */
#define CLR_CRAB_BODY   C2D_Color32(160, 25,  25,  255)  /* dark red */
#define CLR_CRAB_CLAW   C2D_Color32(110, 15,  15,  255)  /* darker red */
#define CLR_SNAIL_SHELL  C2D_Color32(180, 130, 50,  255)  /* tan shell */
#define CLR_SNAIL_SPIRAL C2D_Color32(110, 70,  20,  255)  /* dark spiral */
#define CLR_SNAIL_BODY   C2D_Color32(70,  110, 30,  255)  /* olive body */
/* HUD */
#define CLR_HUD_TEXT    C2D_Color32(255, 255, 255, 255)
#define CLR_HUD_LABEL   C2D_Color32(200, 200, 100, 255)
#define CLR_COIN_YELLOW C2D_Color32(255, 220, 0,   255)  /* kept for compatibility */
#define CLR_NUT         C2D_Color32(180, 180, 180, 255)  /* nut: light gray */
#define CLR_NUT_DARK    C2D_Color32(110, 110, 110, 255)  /* nut: dark gray (hex face) */

/* ── Font rendering (same pattern as lunarrunner) ── */

typedef struct { u32 color; float z; float scale; } TextCtx;

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
    float w = pw * scale;
    float h = FONT_H * scale;
    draw_text(cx - w / 2.0f, cy - h / 2.0f, scale, color, str);
}

/* ─────────────────────────────────────────────────────────────
 * Background
 * ───────────────────────────────────────────────────────────── */

static void draw_space_bg(int cam_x, int cam_y) {
    /* Vertical parallax: how far the camera has panned UP from the ground
       (0 on the ground, grows as the player jumps/climbs). Far background
       objects shift DOWN by a fraction of it — stars least, planet a bit more. */
    int max_cy = LEVEL_MAX_H * TILE_PX - SCREEN_H;
    if (max_cy < 0) max_cy = 0;
    int up = max_cy - cam_y;
    if (up < 0) up = 0;
    float vshift_star   = up * 0.22f;
    float vshift_planet = up * 0.34f;
    const int STAR_DROP = 26;   /* push the whole starfield lower on screen */

    /* ── Stars: two parallax layers ──
       Each layer tiles over a 512-px virtual canvas.
       Far stars (1×1): divisor 8  → tile every ~4096 cam-px (more than a level)
       Near stars (2×2): divisor 5 → tile every ~2560 cam-px                   */
    static const struct { int x; int y; int sz; int spd; } stars[] = {
        /* far: 1×1, spd=8 */
        { 30, 12, 1, 8}, { 82,  7, 1, 8}, {140, 22, 1, 8}, {195,  5, 1, 8},
        {250, 18, 1, 8}, {308, 35, 1, 8}, {370, 10, 1, 8}, { 55, 40, 1, 8},
        {115, 50, 1, 8}, {175, 30, 1, 8}, {230, 45, 1, 8}, {290, 25, 1, 8},
        {345, 55, 1, 8}, {410, 15, 1, 8}, {460, 42, 1, 8}, {500,  8, 1, 8},
        /* near: 2×2, spd=5 */
        { 65, 20, 2, 5}, {160, 38, 2, 5}, {255, 14, 2, 5}, {350, 48, 2, 5},
        {105, 55, 2, 5}, {205, 10, 2, 5}, {300, 42, 2, 5}, {430, 28, 2, 5},
    };
    static const int RANGE = 512;
    int n = (int)(sizeof(stars) / sizeof(stars[0]));

    for (int i = 0; i < n; i++) {
        int shift = (cam_x / stars[i].spd) % RANGE;
        int sx    = (stars[i].x - shift + RANGE * 4) % RANGE;
        u32 color = (stars[i].sz == 1) ? CLR_STAR_FAR : CLR_STAR_NEAR;
        /* draw at sx and sx - RANGE for seamless tile */
        float fy = (float)stars[i].y + STAR_DROP + vshift_star;
        for (int w = 0; w <= 1; w++) {
            float fx = (float)(sx - w * RANGE);
            if (fx > -(float)stars[i].sz && fx < (float)SCREEN_W
                && fy < (float)SCREEN_H)
                C2D_DrawRectSolid(fx, fy, 0.03f,
                                  (float)stars[i].sz, (float)stars[i].sz, color);
        }
    }

    /* ── Distant celestial bodies (AI sprites): slow parallax, wrapping ──
       Each wraps over its own virtual range so only one or two are on screen at
       a time. Far bodies use a bigger divisor (move slower) and share the
       vertical parallax with the stars. Drawn behind the stars (z 0.02). */
    if (g_sheet) {
        static const struct { int idx, bx, by, hdiv, wrap; } bg[] = {
            { ATLAS_BG_MOON,    60, 16, 20, 640 },
            { ATLAS_BG_PLANET2, 300, 22, 16, 780 },
            { ATLAS_BG_NEBULA,  560, 40, 12, 920 },
        };
        for (int b = 0; b < (int)(sizeof(bg)/sizeof(bg[0])); b++) {
            C2D_Image img = C2D_SpriteSheetGetImage(g_sheet, bg[b].idx);
            float iw = (float)img.subtex->width;
            int shift = (cam_x / bg[b].hdiv) % bg[b].wrap;
            int base  = (bg[b].bx - shift + bg[b].wrap * 4) % bg[b].wrap;
            float fy  = (float)bg[b].by + vshift_planet;
            for (int w = 0; w <= 1; w++) {
                float fx = (float)(base - w * bg[b].wrap);
                if (fx > -iw && fx < (float)SCREEN_W)
                    C2D_DrawImageAt(img, fx, fy, 0.02f, NULL, 1.0f, 1.0f);
            }
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Ground scenery: distant mountain ridge (procedural) + wrecked-machine
 * props standing on the horizon. Drawn behind the tiles (z < tile 0.2),
 * with horizontal parallax so they read as distant background near the ground.
 * ───────────────────────────────────────────────────────────── */

static void draw_ground_scenery(int cam_x, int cam_y) {
    int gy = (LEVEL_MAX_H - 2) * TILE_PX - cam_y;   /* ground-surface screen Y */
    if (gy <= 0 || gy > SCREEN_H + 80) return;      /* ground off-screen */

    /* ── Distant mountain ridge: procedural jagged peaks (reliable + tileable) ── */
    {
        static const struct { int x, h, hw; } pk[] = {
            { 40, 44, 50}, {150, 66, 62}, {255, 36, 46}, {360, 72, 70},
            {470, 52, 54}, {560, 32, 42},
        };
        u32 mc  = C2D_Color32(30, 26, 54, 255);    /* dark indigo body  */
        u32 rim = C2D_Color32(54, 70, 96, 255);    /* faint cool rim    */
        const int range = 620;
        int shift = (cam_x * 30 / 100) % range;    /* parallax ~0.30    */
        for (int w = -1; w <= 1; w++) {
            for (int i = 0; i < (int)(sizeof(pk)/sizeof(pk[0])); i++) {
                float px = (float)(pk[i].x - shift + w * range);
                if (px + pk[i].hw < 0 || px - pk[i].hw > SCREEN_W) continue;
                float ay = (float)(gy - pk[i].h);
                C2D_DrawTriangle(px - pk[i].hw, (float)gy, mc,
                                 px,            ay,        mc,
                                 px + pk[i].hw, (float)gy, mc, 0.05f);
                C2D_DrawTriangle(px,        ay,        rim,         /* lit left ridge */
                                 px - 5,    ay + 9,    rim,
                                 px + 1,    ay + 7,    rim, 0.051f);
            }
        }
    }

    /* ── Wrecked-machine props standing on the ground, parallax ~0.62 ── */
    if (g_sheet) {
        static const struct { int idx, bx, wrap; } pr[] = {
            { ATLAS_BG_WRECK,  120, 760 },
            { ATLAS_BG_DEBRIS, 470, 880 },
            { ATLAS_BG_RIG,    820, 1010 },
        };
        for (int i = 0; i < (int)(sizeof(pr)/sizeof(pr[0])); i++) {
            C2D_Image img = C2D_SpriteSheetGetImage(g_sheet, pr[i].idx);
            float iw = (float)img.subtex->width;
            float ih = (float)img.subtex->height;
            int shift = (cam_x * 62 / 100) % pr[i].wrap;
            int base  = (pr[i].bx - shift + pr[i].wrap * 8) % pr[i].wrap;
            for (int w = 0; w <= 1; w++) {
                float fx = (float)(base - w * pr[i].wrap);
                if (fx > -iw && fx < (float)SCREEN_W)
                    C2D_DrawImageAt(img, fx, (float)gy - ih + 2.0f, 0.08f,
                                    NULL, 1.0f, 1.0f);
            }
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Tile rendering
 * ───────────────────────────────────────────────────────────── */

static void draw_tile(uint8_t type, float sx, float sy, int tx, int ty, int surface) {
    float ts = (float)TILE_PX;
    /* stable per-CELL hash (depends on both tx and ty so stacked rows differ) */
    unsigned hsh = (unsigned)(tx * 73856093) ^ (unsigned)(ty * 19349663);

    /* Collectible-style tiles use the AI atlas when available; structural tiles
       (ground/brick/blocks/totem) stay procedural — they tile seamlessly. */
    if (g_sheet && (type == TILE_NUT || type == TILE_SHELL)) {
        int idx   = (type == TILE_NUT) ? ATLAS_NUT_0 : ATLAS_SNAIL_SHELL;
        float scl = (type == TILE_NUT) ? NUT_SPRITE_SCALE : 1.0f;
        draw_image_fit(idx, sx, sy, ts, ts, 0.2f, 0, scl, 1.0f, 1.0f, 0.0f);
        return;
    }

    switch (type) {
    case TILE_GROUND: {
        if (surface) {
            /* Surface tile: lit blue-gray rock top, lighter body */
            C2D_DrawRectSolid(sx, sy,            0.2f,   ts, ts,       CLR_ROCK_BODY);
            C2D_DrawRectSolid(sx, sy + ts*0.55f, 0.205f, ts, ts*0.45f, C2D_Color32(42, 54, 66, 255));
            C2D_DrawRectSolid(sx, sy,            0.21f,  ts, 5,        CLR_ROCK_TOP);
            C2D_DrawRectSolid(sx, sy,            0.211f, ts, 2,        C2D_Color32(125, 152, 172, 255));
            /* a couple of pseudo-random surface boulders */
            if ((hsh & 3) == 0)
                C2D_DrawRectSolid(sx + 4 + (hsh >> 5) % (TILE_PX - 12), sy + 4,
                                  0.212f, 6, 4, C2D_Color32(96, 120, 138, 255));
        } else {
            /* Buried tile: darker, no top light, with a faint strata seam */
            C2D_DrawRectSolid(sx, sy,           0.2f,   ts, ts,      C2D_Color32(40, 50, 62, 255));
            C2D_DrawRectSolid(sx, sy + ts*0.5f, 0.205f, ts, ts*0.5f, C2D_Color32(31, 39, 49, 255));
            C2D_DrawRectSolid(sx, sy + 1 + (hsh % 4), 0.206f, ts, 1, C2D_Color32(50, 62, 76, 255));
        }
        /* per-CELL detail — differs every tile, so the two ground layers differ */
        C2D_DrawRectSolid(sx + 2, sy + 6 + (hsh % (TILE_PX - 10)), 0.212f, ts - 6, 1, CLR_ROCK_CRACK);
        C2D_DrawRectSolid(sx + 3 + (hsh >> 3)  % (TILE_PX - 8),
                          sy + 6 + (hsh >> 7)  % (TILE_PX - 10), 0.213f, 2, 2,
                          C2D_Color32(72, 88, 102, 255));
        C2D_DrawRectSolid(sx + 3 + (hsh >> 11) % (TILE_PX - 7),
                          sy + 7 + (hsh >> 15) % (TILE_PX - 11), 0.213f, 1, 1,
                          C2D_Color32(38, 48, 58, 255));
        if ((hsh % 6) == 0)   /* occasional teal mineral fleck */
            C2D_DrawRectSolid(sx + 2 + (hsh >> 19) % (TILE_PX - 6),
                              sy + ts - 7, 0.214f, 2, 2, C2D_Color32(60, 180, 160, 255));
        break;
    }
    case TILE_BRICK:
        /* Metallic panel: steel base + beveled edges + corner rivets */
        C2D_DrawRectSolid(sx,        sy,        0.2f,  ts,    ts,    CLR_METAL_BASE);
        C2D_DrawRectSolid(sx,        sy,        0.21f, ts,    2,     CLR_METAL_LIGHT);
        C2D_DrawRectSolid(sx,        sy,        0.21f, 2,     ts,    CLR_METAL_LIGHT);
        C2D_DrawRectSolid(sx,        sy + ts-2, 0.21f, ts,    2,     CLR_METAL_DARK);
        C2D_DrawRectSolid(sx + ts-2, sy,        0.21f, 2,     ts,    CLR_METAL_DARK);
        C2D_DrawRectSolid(sx + 2,    sy + 2,    0.22f, 2,     2,     CLR_METAL_DARK);
        C2D_DrawRectSolid(sx + ts-4, sy + 2,    0.22f, 2,     2,     CLR_METAL_DARK);
        C2D_DrawRectSolid(sx + 2,    sy + ts-4, 0.22f, 2,     2,     CLR_METAL_DARK);
        C2D_DrawRectSolid(sx + ts-4, sy + ts-4, 0.22f, 2,     2,     CLR_METAL_DARK);
        break;
    case TILE_QBLOCK: {
        /* Supply crate: dark hull + teal glow border + hex-nut icon */
        u32 hull = C2D_Color32(30, 42, 58, 255);
        C2D_DrawRectSolid(sx,      sy,      0.2f,  ts, ts, hull);
        /* Teal glow border */
        C2D_DrawRectSolid(sx,      sy,      0.21f, ts, 2,  CLR_PLANET_HI);
        C2D_DrawRectSolid(sx,      sy+ts-2, 0.21f, ts, 2,  CLR_PLANET_HI);
        C2D_DrawRectSolid(sx,      sy,      0.21f, 2,  ts, CLR_PLANET_HI);
        C2D_DrawRectSolid(sx+ts-2, sy,      0.21f, 2,  ts, CLR_PLANET_HI);
        /* Nut icon: real sprite, centered at ~62% of the tile */
        if (g_sheet) {
            C2D_Image ni = C2D_SpriteSheetGetImage(g_sheet, ATLAS_NUT_0);
            float iw = (float)ni.subtex->width, ih = (float)ni.subtex->height;
            float md = (iw > ih) ? iw : ih;
            float s  = (ts * 0.62f) / md;
            float dw = iw * s, dh = ih * s;
            C2D_DrawImageAt(ni, sx + (ts - dw) * 0.5f, sy + (ts - dh) * 0.5f,
                            0.22f, NULL, s, s);
        } else {
            /* fallback hex-nut, scaled to the tile */
            float c = ts * 0.5f, r = ts * 0.28f;
            C2D_DrawRectSolid(sx + c - r,     sy + c - r,     0.22f, r*2, r*2, CLR_NUT);
            C2D_DrawRectSolid(sx + c - r*0.5f, sy + c - r*0.5f, 0.23f, r, r, CLR_NUT_DARK);
        }
        break;
    }
    case TILE_QBLOCK_USED: {
        /* Depleted crate: dim dark hull, thin gray outline, no glow */
        u32 dim  = C2D_Color32(28, 32, 40, 255);
        u32 edge = C2D_Color32(55, 65, 75, 255);
        C2D_DrawRectSolid(sx,      sy,      0.2f,  ts, ts, dim);
        C2D_DrawRectSolid(sx,      sy,      0.21f, ts, 1,  edge);
        C2D_DrawRectSolid(sx,      sy+ts-1, 0.21f, ts, 1,  edge);
        C2D_DrawRectSolid(sx,      sy,      0.21f, 1,  ts, edge);
        C2D_DrawRectSolid(sx+ts-1, sy,      0.21f, 1,  ts, edge);
        break;
    }
    case TILE_PIPE_TOP:
        /* Totem cap: 2px wider overhang, lighter top edge, orange eye-glyph */
        C2D_DrawRectSolid(sx - 2,      sy,          0.2f,  ts + 4, ts,      CLR_TOTEM_BODY);
        C2D_DrawRectSolid(sx - 2,      sy,          0.21f, ts + 4, 2,       CLR_TOTEM_EDGE);
        C2D_DrawRectSolid(sx - 2,      sy,          0.21f, 2,      ts,      CLR_TOTEM_EDGE);
        C2D_DrawRectSolid(sx + ts,     sy,          0.21f, 2,      ts,      CLR_TOTEM_EDGE);
        /* Eye rune: orange rect, dark pupil inside */
        C2D_DrawRectSolid(sx + ts*0.22f, sy+ts*0.28f, 0.22f, ts*0.56f, ts*0.44f, CLR_TOTEM_GLYPH);
        C2D_DrawRectSolid(sx + ts*0.34f, sy+ts*0.38f, 0.23f, ts*0.32f, ts*0.24f, CLR_TOTEM_BODY);
        break;
    case TILE_PIPE_BODY:
        /* Totem body: dark stone, edge highlights, horizontal glyph stripe */
        C2D_DrawRectSolid(sx,      sy,          0.2f,  ts, ts,  CLR_TOTEM_BODY);
        C2D_DrawRectSolid(sx,      sy,          0.21f, 2,  ts,  CLR_TOTEM_EDGE);
        C2D_DrawRectSolid(sx+ts-2, sy,          0.21f, 2,  ts,  CLR_TOTEM_EDGE);
        /* Orange glyph stripe at tile midpoint */
        C2D_DrawRectSolid(sx + 3,  sy+ts/2 - 1, 0.22f, ts-6, 2, CLR_TOTEM_GLYPH);
        break;
    case TILE_SOLID:
        C2D_DrawRectSolid(sx, sy, 0.2f, ts, ts, CLR_PLATFORM);
        C2D_DrawRectSolid(sx, sy, 0.21f, ts, 2, CLR_PLATFORM_D);
        break;
    case TILE_NUT:
        /* Hex nut: gray body with dark center hole */
        C2D_DrawRectSolid(sx + ts/4,     sy + ts/4, 0.2f, ts/2, ts/2, CLR_NUT);
        C2D_DrawRectSolid(sx + ts/4 + 2, sy + ts/4 + 2, 0.21f, ts/2 - 4, ts/2 - 4, CLR_NUT_DARK);
        C2D_DrawRectSolid(sx + ts/4 + 4, sy + ts/4 + 4, 0.22f, ts/2 - 8, ts/2 - 8, CLR_NUT);
        break;
    case TILE_SHELL:
        /* Shell dome on the ground */
        C2D_DrawRectSolid(sx + ts*0.1f,  sy + ts*0.2f, 0.2f,  ts*0.8f, ts*0.65f, CLR_SNAIL_SHELL);
        C2D_DrawRectSolid(sx + ts*0.35f, sy + ts*0.32f, 0.21f, ts*0.3f, ts*0.22f, CLR_SNAIL_SPIRAL);
        break;
    default:
        break;
    }
}

static void draw_tilemap(const Game *g) {
    const Level *lvl = &g->level;
    int cam_x = lvl->cam_x;
    int cam_y = lvl->cam_y;

    int tx0 = cam_x / TILE_PX;
    int tx1 = tx0 + SCREEN_W / TILE_PX + 2;

    for (int ty = 0; ty < lvl->height; ty++) {
        float sy = (float)(ty * TILE_PX - cam_y);
        if (sy + TILE_PX < 0 || sy >= SCREEN_H) continue;   /* off-screen row */
        for (int tx = tx0; tx <= tx1; tx++) {
            uint8_t tile = level_tile(lvl, tx, ty);
            if (tile == TILE_AIR) continue;
            float sx = (float)(tx * TILE_PX - cam_x);
            int surface = (level_tile(lvl, tx, ty - 1) != TILE_GROUND);
            draw_tile(tile, sx, sy, tx, ty, surface);
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Durio
 * ───────────────────────────────────────────────────────────── */

static void draw_durio(const Game *g) {
    const Durio *m = &g->durio;
    if (m->state == DURIO_DEAD && (m->dead_timer / 80) & 1) return; /* blink */

    int cam_x = g->level.cam_x;
    int cam_y = g->level.cam_y;
    float sx  = (float)durio_screen_x(m, cam_x);
    float sy  = (float)durio_screen_y(m) - cam_y;
    float w   = (float)DURIO_W;
    float h   = (float)durio_height(m);
    float z   = 0.5f;

    if (g_sheet) {
        int flip = (m->dir == DURIO_FACE_LEFT);     /* art faces right */
        int idx;
        static const int wk[3] = { ATLAS_DURIO_WALK0, ATLAS_DURIO_WALK1,
                                   ATLAS_DURIO_WALK2 };
        if (m->state == DURIO_DEAD)                  idx = ATLAS_DURIO_DEAD;
        else if (!m->on_ground)                      idx = ATLAS_DURIO_JUMP;
        else if (m->vx_fp > FP_ONE || m->vx_fp < -FP_ONE)
                                                     idx = wk[m->walk_frame % 3];
        else                                         idx = ATLAS_DURIO_STAND;
        draw_image_fit(idx, sx, sy, w, h, z, flip,
                       DURIO_SPRITE_SCALE, 1.0f, 1.0f, 0.0f);
        return;
    }

    /*
     * Alien space explorer — 16×16:
     *   y  0-2   antenna (stem + ball)
     *   y  2-10  round helmet (cyan) with dark visor strip + highlight
     *   y 10-13  spacesuit body (purple)
     *   y 13-16  rocket boots (orange, alternate up/down while walking)
     */

    /* ── Antenna ── */
    C2D_DrawRectSolid(sx + w*0.44f, sy,           z,       2,       h*0.18f, CLR_DURIO_ANTENNA);
    C2D_DrawRectSolid(sx + w*0.30f, sy,           z,       w*0.28f, h*0.12f, CLR_DURIO_ANTENNA);

    /* ── Helmet ── */
    C2D_DrawRectSolid(sx + w*0.12f, sy + h*0.12f, z,       w*0.76f, h*0.50f, CLR_DURIO_HEAD);

    /* ── Visor strip ── */
    C2D_DrawRectSolid(sx + w*0.12f, sy + h*0.28f, z+0.01f, w*0.76f, h*0.20f, CLR_DURIO_VISOR);
    /* Visor highlight: small bright dot, side depends on facing direction */
    float hi_x = (m->dir == DURIO_FACE_RIGHT) ? sx + w*0.18f : sx + w*0.56f;
    C2D_DrawRectSolid(hi_x,         sy + h*0.30f, z+0.02f, 2,       2,       CLR_WHITE);

    /* ── Spacesuit body ── */
    C2D_DrawRectSolid(sx + w*0.18f, sy + h*0.62f, z,       w*0.64f, h*0.22f, CLR_DURIO_BODY);

    /* ── Rocket boots (alternate up/down each walk frame) ── */
    float bl = 0.0f, br = 0.0f;
    if (m->on_ground) {
        if (m->walk_frame == 1) { bl = -1.5f; br =  1.5f; }
        if (m->walk_frame == 2) { bl =  1.5f; br = -1.5f; }
    }
    C2D_DrawRectSolid(sx + w*0.06f, sy + h*0.84f + bl, z, w*0.36f, h*0.16f, CLR_DURIO_BOOTS);
    C2D_DrawRectSolid(sx + w*0.58f, sy + h*0.84f + br, z, w*0.36f, h*0.16f, CLR_DURIO_BOOTS);
}

/* ─────────────────────────────────────────────────────────────
 * Enemies
 * ───────────────────────────────────────────────────────────── */

static void draw_enemies(const Game *g) {
    int cam_x = g->level.cam_x;
    int cam_y = g->level.cam_y;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &g->enemies[i];
        if (e->type == ENEMY_NONE || e->status == ENEMY_DEAD) continue;

        float sx = (float)(FP_TO_INT(e->x_fp) - cam_x);
        float sy = (float)FP_TO_INT(e->y_fp) - cam_y;
        float w  = (float)ENEMY_W;
        float z  = 0.45f;

        if (sx + w < 0 || sx > SCREEN_W) continue; /* off-screen */

        if (g_sheet) {
            int flip = (e->vx_fp > 0);              /* art faces left */
            float h  = (float)ENEMY_H;
            int anim = (FP_TO_INT(e->x_fp) >> 3) & 1;  /* 2-frame gait, tied to movement */
            if (e->type == ENEMY_CRAB) {
                if (e->status == ENEMY_SQUISHED)
                    draw_image_fit(ATLAS_CRAB_WALK0, sx, sy, w, h, z, flip,
                                   ENEMY_SPRITE_SCALE, 1.15f, 0.42f, 0.0f);
                else
                    draw_image_fit(anim ? ATLAS_CRAB_WALK1 : ATLAS_CRAB_WALK0,
                                   sx, sy, w, h, z, flip, ENEMY_SPRITE_SCALE, 1.0f, 1.0f, 0.0f);
            } else if (e->type == ENEMY_SNAIL) {
                if (e->status == ENEMY_SHELL)
                    draw_image_fit(ATLAS_SNAIL_SHELL, sx, sy, w, h, z, 0,
                                   ENEMY_SPRITE_SCALE, 1.0f, 1.0f, 0.0f);
                else
                    draw_image_fit(anim ? ATLAS_SNAIL_WALK1 : ATLAS_SNAIL_WALK0,
                                   sx, sy, w, h, z, flip, ENEMY_SPRITE_SCALE, 1.0f, 1.0f, 0.0f);
            }
            continue;
        }

        switch (e->type) {
        case ENEMY_CRAB: {
            float h = (e->status == ENEMY_SQUISHED)
                      ? (float)ENEMY_H_SQUISH : (float)ENEMY_H;
            if (e->status == ENEMY_SQUISHED) {
                /* Flat squished crab */
                C2D_DrawRectSolid(sx, sy, z, w, h, CLR_CRAB_BODY);
            } else {
                /* Body: wide squat rect */
                C2D_DrawRectSolid(sx + w*0.15f, sy + h*0.25f, z,       w*0.70f, h*0.55f, CLR_CRAB_BODY);
                /* Left claw */
                C2D_DrawRectSolid(sx,           sy + h*0.30f, z,       w*0.20f, h*0.35f, CLR_CRAB_CLAW);
                /* Right claw */
                C2D_DrawRectSolid(sx + w*0.80f, sy + h*0.30f, z,       w*0.20f, h*0.35f, CLR_CRAB_CLAW);
                /* Eyes */
                C2D_DrawRectSolid(sx + w*0.28f, sy + h*0.28f, z+0.01f, 2, 2,    CLR_WHITE);
                C2D_DrawRectSolid(sx + w*0.62f, sy + h*0.28f, z+0.01f, 2, 2,    CLR_WHITE);
                /* Legs */
                C2D_DrawRectSolid(sx + w*0.20f, sy + h*0.80f, z,       w*0.60f, h*0.18f, CLR_CRAB_CLAW);
            }
            break;
        }
        case ENEMY_SNAIL: {
            float h = (float)ENEMY_H;
            /* Shell dome (top 65%) */
            C2D_DrawRectSolid(sx + w*0.10f, sy,           z,       w*0.80f, h*0.65f, CLR_SNAIL_SHELL);
            /* Spiral in shell center */
            C2D_DrawRectSolid(sx + w*0.35f, sy + h*0.18f, z+0.01f, w*0.30f, h*0.22f, CLR_SNAIL_SPIRAL);
            /* Slug body (bottom 35%) */
            C2D_DrawRectSolid(sx,           sy + h*0.65f,  z,       w,       h*0.35f, CLR_SNAIL_BODY);
            /* Tentacles at front */
            float tx_off = (e->vx_fp >= 0) ? w*0.62f : w*0.10f;
            C2D_DrawRectSolid(sx + tx_off,        sy - 3, z+0.01f, 2, 4, CLR_SNAIL_BODY);
            C2D_DrawRectSolid(sx + tx_off + 4,    sy - 3, z+0.01f, 2, 4, CLR_SNAIL_BODY);
            break;
        }
        default:
            break;
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Coins
 * ───────────────────────────────────────────────────────────── */

static void draw_nuts(const Game *g) {
    int cam_x = g->level.cam_x;
    int cam_y = g->level.cam_y;

    for (int i = 0; i < MAX_NUTS; i++) {
        const Nut *c = &g->nuts[i];
        if (!c->active) continue;

        float sx = (float)(FP_TO_INT(c->x_fp) - cam_x);
        float sy = (float)FP_TO_INT(c->y_fp) - cam_y;

        /* Tumble animation: alternate wide/narrow (nut spinning) */
        int frame = (c->anim_timer / 80) % 4;

        if (g_sheet) {
            float sxm = (frame == 1 || frame == 3) ? 0.35f : 1.0f;   /* spin */
            float box = (float)TILE_PX;
            draw_image_fit(ATLAS_NUT_0, sx, sy, box, box, 0.4f, 0,
                           NUT_SPRITE_SCALE, sxm, 1.0f, 0.0f);
            continue;
        }

        float nw  = (frame == 1 || frame == 3) ? 4.0f : 10.0f;
        float nx_off = (TILE_PX - nw) / 2.0f;
        float nh  = 10.0f;

        C2D_DrawRectSolid(sx + nx_off,     sy + 3, 0.4f,  nw,     nh,     CLR_NUT);
        C2D_DrawRectSolid(sx + nx_off + 2, sy + 5, 0.41f, nw - 4, nh - 4, CLR_NUT_DARK);
    }
}

/* ─────────────────────────────────────────────────────────────
 * Overlays
 * ───────────────────────────────────────────────────────────── */

static void draw_overlay(const Game *g) {
    switch (g->state) {
    case GAME_TITLE:
        C2D_DrawRectSolid(0, 0, 0.9f, SCREEN_W, SCREEN_H, C2D_Color32(0, 0, 0, 140));
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f - 50, 5.0f, CLR_RED,    "SUPER");
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f - 10, 5.0f, CLR_YELLOW, "DURIO");
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 40, 2.5f, CLR_WHITE,  "PRESS START");
        break;
    case GAME_PAUSED:
        C2D_DrawRectSolid(0, 0, 0.9f, SCREEN_W, SCREEN_H, CLR_DIM);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f, 4.0f, CLR_WHITE, "PAUSED");
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 35, 2.0f, CLR_GRAY, "START to resume");
        break;
    case GAME_DEAD:
        C2D_DrawRectSolid(0, 0, 0.9f, SCREEN_W, SCREEN_H, CLR_DIM);
        draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f - 20, 4.0f, CLR_RED, "GAME OVER");
        if (g->durio.lives > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Lives: %d", g->durio.lives);
            draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 20, 2.5f, CLR_WHITE, buf);
            draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 48, 2.0f, CLR_GRAY, "A to retry");
        } else {
            draw_text_centered(SCREEN_W / 2.0f, SCREEN_H / 2.0f + 20, 2.5f, CLR_GRAY, "A for title");
        }
        break;
    default:
        /* HUD score during play */
        if (g->state == GAME_PLAYING) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%06d", g->durio.score);
            draw_text(8, 6, 2.0f, CLR_WHITE, buf);
        }
        break;
    }
}

/* ─────────────────────────────────────────────────────────────
 * Top screen
 * ───────────────────────────────────────────────────────────── */

static void render_top_3ds(const Game *g) {
    /* Space sky: uniform deep space (background objects carry the depth now) */
    C2D_DrawRectSolid(0, 0, 0.01f, SCREEN_W, SCREEN_H, CLR_SKY);

    draw_space_bg(g->level.cam_x, g->level.cam_y);
    draw_ground_scenery(g->level.cam_x, g->level.cam_y);
    draw_tilemap(g);
    draw_nuts(g);
    draw_enemies(g);

    if (g->state != GAME_TITLE)
        draw_durio(g);

    draw_overlay(g);

#if DD_DEBUG
    {
        int cx = g->level.cam_x, cy = g->level.cam_y;
        u32 cyan  = C2D_Color32(0, 255, 255, 255);
        u32 green = C2D_Color32(0, 255, 0, 255);
        u32 yellow= C2D_Color32(255, 255, 0, 255);
        u32 mag   = C2D_Color32(255, 0, 255, 255);
        const Durio *m = &g->durio;
        /* cyan hitbox of durio */
        if (g->state != GAME_TITLE) {
            float bx = (float)durio_screen_x(m, cx);
            float by = (float)FP_TO_INT(m->y_fp) - cy;
            C2D_DrawRectSolid(bx, by, 0.7f, DURIO_W, 1, cyan);
            C2D_DrawRectSolid(bx, by + DURIO_H - 1, 0.7f, DURIO_W, 1, cyan);
            C2D_DrawRectSolid(bx, by, 0.7f, 1, DURIO_H, cyan);
            C2D_DrawRectSolid(bx + DURIO_W - 1, by, 0.7f, 1, DURIO_H, cyan);
        }
        /* magenta hitboxes of enemies */
        for (int i = 0; i < MAX_ENEMIES; i++) {
            const Enemy *e = &g->enemies[i];
            if (e->type == ENEMY_NONE || e->status == ENEMY_DEAD) continue;
            float bx = (float)(FP_TO_INT(e->x_fp) - cx);
            float by = (float)FP_TO_INT(e->y_fp) - cy;
            float eh = (e->status == ENEMY_SQUISHED) ? ENEMY_H_SQUISH : ENEMY_H;
            C2D_DrawRectSolid(bx, by, 0.7f, ENEMY_W, 1, mag);
            C2D_DrawRectSolid(bx, by + eh - 1, 0.7f, ENEMY_W, 1, mag);
            C2D_DrawRectSolid(bx, by, 0.7f, 1, eh, mag);
            C2D_DrawRectSolid(bx + ENEMY_W - 1, by, 0.7f, 1, eh, mag);
        }

        char db[64];
        snprintf(db, sizeof(db), "%s X%d VX%d", DD_VERSION,
                 (int)FP_TO_INT(m->x_fp), (int)FP_TO_INT(m->vx_fp));
        draw_text(4, 30, 2.2f, green, db);
        snprintf(db, sizeof(db), "CX%d MX%d OG%d", g->level.cam_x,
                 (int)FP_TO_INT(g->max_player_x_fp), m->on_ground);
        draw_text(4, 52, 2.2f, yellow, db);
    }
#endif
}

/* ─────────────────────────────────────────────────────────────
 * Bottom screen HUD
 * ───────────────────────────────────────────────────────────── */

static void render_bottom_3ds(const Game *g) {
    C2D_DrawRectSolid(0, 0, 0.1f, BOT_SCREEN_W, BOT_SCREEN_H, CLR_BOT_BG);

    float y = 10;

    draw_text(8, y, 2.5f, CLR_RED, "DURIO"); y += 24;

    char buf[48];
    snprintf(buf, sizeof(buf), "SCORE  %06d", g->durio.score);
    draw_text(8, y, 2.0f, CLR_HUD_TEXT, buf); y += 18;

    snprintf(buf, sizeof(buf), "NUTS   x%02d", g->durio.nuts);
    draw_text(8, y, 2.0f, CLR_NUT, buf); y += 18;

    snprintf(buf, sizeof(buf), "LIVES  x%d", g->durio.lives);
    draw_text(8, y, 2.0f, CLR_WHITE, buf); y += 18;

    /* Separator */
    C2D_DrawRectSolid(8, y, 0.5f, BOT_SCREEN_W - 16, 1, CLR_GRAY); y += 8;

    /* High score */
    snprintf(buf, sizeof(buf), "BEST   %06d", g->save.high_score);
    draw_text(8, y, 1.8f, CLR_HUD_LABEL, buf); y += 16;

    /* Controls hint */
    float by = BOT_SCREEN_H - 22;
    switch (g->state) {
    case GAME_TITLE:
        draw_text(8, by, 1.8f, CLR_GRAY, "START: Play");
        break;
    case GAME_PLAYING:
        draw_text(8, by, 1.5f, CLR_GRAY, "DPAD:Move  A/B:Jump  L:Run");
        break;
    case GAME_PAUSED:
        draw_text(8, by, 1.8f, CLR_GRAY, "START: Resume  B: Quit");
        break;
    case GAME_DEAD:
        draw_text(8, by, 1.8f, CLR_RED, "GAME OVER  A: Title");
        break;
    }
}

/* ─────────────────────────────────────────────────────────────
 * Public API
 * ───────────────────────────────────────────────────────────── */

void render_init_3ds(void) {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    /* Load the AI-generated entity atlas from romfs. On failure g_sheet stays
       NULL and every entity falls back to its procedural rectangle drawing. */
    romfsInit();
    g_sheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
}

void render_exit_3ds(void) {
    if (g_sheet) C2D_SpriteSheetFree(g_sheet);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
}

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
