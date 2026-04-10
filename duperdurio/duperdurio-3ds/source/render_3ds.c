#include "render_3ds.h"
#include "../../duperdurio-common/font.h"
#include "../../duperdurio-common/config.h"
#include "../../duperdurio-common/level.h"
#include <stdio.h>
#include <string.h>

/*
 * 3DS renderer using citro2d.
 * All game objects drawn as colored rectangles — no external textures.
 * citro2d cap: ~4096 draw calls/frame; this stays well under 400.
 */

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

static void draw_space_bg(int cam_x) {
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
        for (int w = 0; w <= 1; w++) {
            float fx = (float)(sx - w * RANGE);
            if (fx > -(float)stars[i].sz && fx < (float)SCREEN_W)
                C2D_DrawRectSolid(fx, (float)stars[i].y, 0.03f,
                                  (float)stars[i].sz, (float)stars[i].sz, color);
        }
    }

    /* ── Planet: teal sphere, parallax 1/8, wrapping ──
       Rendered as horizontal strips (2 px tall) to approximate a circle.
       Wraps on a 440-px virtual canvas (SCREEN_W + planet_diameter):
       exits left edge → reappears from right.                             */
    {
        static const struct { int dy; int hw; } pl[] = {
            {-18,  8}, {-16, 12}, {-14, 15}, {-12, 17},
            {-10, 18}, { -8, 19}, { -6, 20}, { -4, 20},
            { -2, 20}, {  0, 20}, {  2, 20}, {  4, 20},
            {  6, 19}, {  8, 18}, { 10, 17}, { 12, 15},
            { 14, 12}, { 16,  8},
        };
        int npl = (int)(sizeof(pl) / sizeof(pl[0]));
        /* Planet wraps: period = SCREEN_W + planet_diameter (440).
           When it exits left it reappears from the right seamlessly. */
        static const int PLANET_RANGE = 440;
        int pshift   = (cam_x / 8) % PLANET_RANGE;
        int pcx_base = (350 - pshift + PLANET_RANGE * 4) % PLANET_RANGE;
        int pcy = 40;

        for (int w = 0; w <= 1; w++) {
            int pcx = pcx_base - w * PLANET_RANGE;
            for (int i = 0; i < npl; i++) {
                int ry = pcy + pl[i].dy;
                if (ry < 0 || ry >= SCREEN_H) continue;
                if (pcx + pl[i].hw < 0 || pcx - pl[i].hw >= SCREEN_W) continue;
                u32 c = (pl[i].dy < -8) ? CLR_PLANET_HI  :
                        (pl[i].dy >  8) ? CLR_PLANET_DARK : CLR_PLANET_MID;
                C2D_DrawRectSolid((float)(pcx - pl[i].hw), (float)ry,
                                  0.02f, (float)(pl[i].hw * 2), 2.0f, c);
            }
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Tile rendering
 * ───────────────────────────────────────────────────────────── */

static void draw_tile(uint8_t type, float sx, float sy) {
    float ts = (float)TILE_PX;
    switch (type) {
    case TILE_GROUND:
        /* Alien rock: dark body + lighter blue-gray surface + crack seam */
        C2D_DrawRectSolid(sx, sy,          0.2f,  ts,  ts,     CLR_ROCK_BODY);
        C2D_DrawRectSolid(sx, sy,          0.21f, ts,  4,      CLR_ROCK_TOP);
        C2D_DrawRectSolid(sx, sy + ts/2,   0.21f, ts,  1,      CLR_ROCK_CRACK);
        C2D_DrawRectSolid(sx, sy + ts - 1, 0.21f, ts,  1,      CLR_ROCK_CRACK);
        break;
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
        /* Hex-nut icon: three strips forming hexagon + center hole */
        C2D_DrawRectSolid(sx + 5,  sy + 4,  0.22f, 6, 2, CLR_NUT);
        C2D_DrawRectSolid(sx + 4,  sy + 6,  0.22f, 8, 4, CLR_NUT);
        C2D_DrawRectSolid(sx + 5,  sy + 10, 0.22f, 6, 2, CLR_NUT);
        C2D_DrawRectSolid(sx + 6,  sy + 7,  0.23f, 4, 2, hull);  /* hole */
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

    int tx0 = cam_x / TILE_PX;
    int tx1 = tx0 + SCREEN_W / TILE_PX + 2;

    for (int ty = 0; ty < lvl->height; ty++) {
        float sy = (float)(ty * TILE_PX);
        for (int tx = tx0; tx <= tx1; tx++) {
            uint8_t tile = level_tile(lvl, tx, ty);
            if (tile == TILE_AIR) continue;
            float sx = (float)(tx * TILE_PX - cam_x);
            draw_tile(tile, sx, sy);
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
    float sx  = (float)durio_screen_x(m, cam_x);
    float sy  = (float)durio_screen_y(m);
    float w   = (float)DURIO_W;
    float h   = (float)durio_height(m);
    float z   = 0.5f;

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

    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &g->enemies[i];
        if (e->type == ENEMY_NONE || e->status == ENEMY_DEAD) continue;

        float sx = (float)(FP_TO_INT(e->x_fp) - cam_x);
        float sy = (float)FP_TO_INT(e->y_fp);
        float w  = (float)ENEMY_W;
        float z  = 0.45f;

        if (sx + w < 0 || sx > SCREEN_W) continue; /* off-screen */

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

    for (int i = 0; i < MAX_NUTS; i++) {
        const Nut *c = &g->nuts[i];
        if (!c->active) continue;

        float sx = (float)(FP_TO_INT(c->x_fp) - cam_x);
        float sy = (float)FP_TO_INT(c->y_fp);

        /* Tumble animation: alternate wide/narrow (nut spinning) */
        int frame = (c->anim_timer / 80) % 4;
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
    /* Space sky: dark top, subtle purple near horizon */
    C2D_DrawRectSolid(0, 0,              0.01f, SCREEN_W, SCREEN_H * 5 / 8, CLR_SKY);
    C2D_DrawRectSolid(0, SCREEN_H*5/8,  0.01f, SCREEN_W, SCREEN_H * 3 / 8, CLR_SKY_BOTTOM);

    draw_space_bg(g->level.cam_x);
    draw_tilemap(g);
    draw_nuts(g);
    draw_enemies(g);

    if (g->state != GAME_TITLE)
        draw_durio(g);

    draw_overlay(g);
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
        draw_text(8, by, 1.5f, CLR_GRAY, "DPAD:Move  A/B:Jump  Y:Run");
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
}

void render_exit_3ds(void) {
    C2D_Fini();
    C3D_Fini();
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
