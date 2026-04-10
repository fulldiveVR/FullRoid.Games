#include "game.h"
#include "map_gen.h"
#include <string.h>
#include <stddef.h>

#define DEAD_WAIT_MS    1500   /* ms after death before restart prompt */
#define CAM_DURIO_X     80     /* where Durio sits horizontally on screen */

/* ─────────────────────────────────────────────────────────────
 * Enemy spawning for one buffer block
 * ───────────────────────────────────────────────────────────── */

static void spawn_block_enemies(Game *g, int buf_block) {
    int world_block = g->base_block_idx + buf_block;
    int pat_id;
    switch (buf_block) {
        case 0:  pat_id = g->prev_pattern; break;
        case 1:  pat_id = g->curr_pattern; break;
        default: pat_id = g->next_pattern; break;
    }
    const Pattern *pat = &g_patterns[pat_id];
    for (int i = 0; i < pat->enemy_count; i++) {
        int wx = (world_block * BLOCK_W + pat->enemies[i].tx) * TILE_PX;
        int wy = (pat->enemies[i].ty - 1) * TILE_PX - ENEMY_H;
        for (int s = 0; s < MAX_ENEMIES; s++) {
            if (g->enemies[s].type == ENEMY_NONE) {
                enemy_spawn(&g->enemies[s], pat->enemies[i].type, wx, wy);
                break;
            }
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Init / start
 * ───────────────────────────────────────────────────────────── */

void game_init(Game *g) {
    memset(g, 0, sizeof(*g));
    g->state = GAME_TITLE;
    save_load(&g->save);
    mapgen_init_patterns();
    int ids[3];
    mapgen_init_buffer(g->map_tiles, ids);
    g->prev_pattern   = ids[0];
    g->curr_pattern   = ids[1];
    g->next_pattern   = ids[2];
    g->base_block_idx = 0;
    level_init(&g->level, g->map_tiles, MAP_BUF_W, LEVEL_MAX_H);
}

static void game_respawn(Game *g) {
    int32_t death_x = g->durio.x_fp;
    g->dead_wait = 0;
    g->state     = GAME_PLAYING;
    durio_respawn(&g->durio, death_x);
}

void game_start(Game *g) {
    memset(g->map_tiles, 0, sizeof(g->map_tiles));

    int ids[3];
    mapgen_init_buffer(g->map_tiles, ids);
    g->prev_pattern    = ids[0];
    g->curr_pattern    = ids[1];
    g->next_pattern    = ids[2];
    g->base_block_idx  = 0;
    g->max_player_x_fp = 0;

    level_init(&g->level, g->map_tiles, MAP_BUF_W, LEVEL_MAX_H);
    g->level.cam_x = 0;

    durio_init(&g->durio);

    for (int i = 0; i < MAX_ENEMIES; i++) g->enemies[i].type = ENEMY_NONE;
    spawn_block_enemies(g, 1);
    spawn_block_enemies(g, 2);

    for (int i = 0; i < MAX_NUTS; i++) g->nuts[i].active = 0;

    g->dead_wait     = 0;
    g->respawn_timer = 0;
    g->state         = GAME_PLAYING;
}

/* ─────────────────────────────────────────────────────────────
 * Block advance: triggered when player enters the next block
 * ───────────────────────────────────────────────────────────── */

static void check_block_advance(Game *g) {
    int player_tx = level_px_to_tx(FP_TO_INT(g->durio.x_fp));
    int next_block_start_tx = (g->base_block_idx + 2) * BLOCK_W;
    if (player_tx < next_block_start_tx) return;

    /* 1. Shift tile buffer left by one block (row by row) */
    for (int row = 0; row < LEVEL_MAX_H; row++) {
        memmove(
            g->map_tiles + row * MAP_BUF_W,
            g->map_tiles + row * MAP_BUF_W + BLOCK_W,
            2 * BLOCK_W
        );
    }

    /* 2. Advance counters */
    g->base_block_idx++;
    g->level.base_block_idx = g->base_block_idx;
    g->prev_pattern = g->curr_pattern;
    g->curr_pattern = g->next_pattern;
    g->next_pattern = mapgen_pick_next(g->curr_pattern);

    /* 3. Write new pattern into buf block 2 */
    mapgen_write_block(g->map_tiles, 2, &g_patterns[g->next_pattern]);

    /* 4. Despawn entities that fell off the left edge */
    int min_world_px = g->base_block_idx * BLOCK_W * TILE_PX;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies[i].type != ENEMY_NONE &&
            FP_TO_INT(g->enemies[i].x_fp) < min_world_px)
            g->enemies[i].type = ENEMY_NONE;
    }
    for (int i = 0; i < MAX_NUTS; i++) {
        if (g->nuts[i].active &&
            FP_TO_INT(g->nuts[i].x_fp) < min_world_px)
            g->nuts[i].active = 0;
    }

    /* 5. Spawn enemies for the new next block */
    spawn_block_enemies(g, 2);
}

/* ─────────────────────────────────────────────────────────────
 * Camera + back-scroll clamp
 * ───────────────────────────────────────────────────────────── */

static void update_camera(Game *g) {
    /* Track furthest position reached */
    if (g->durio.x_fp > g->max_player_x_fp)
        g->max_player_x_fp = g->durio.x_fp;

    /* Clamp player to back-scroll limit */
    int32_t min_x_fp = g->max_player_x_fp - INT_TO_FP(BACKSCROLL_PX);
    if (min_x_fp < 0) min_x_fp = 0;
    if (g->durio.x_fp < min_x_fp) {
        g->durio.x_fp = min_x_fp;
        if (g->durio.vx_fp < 0) g->durio.vx_fp = 0;
    }

    /* Camera follows player */
    int durio_wx = FP_TO_INT(g->durio.x_fp);
    int target   = durio_wx - CAM_DURIO_X;
    if (target < 0) target = 0;
    g->level.cam_x = target;
}

/* ─────────────────────────────────────────────────────────────
 * Durio <-> enemy collisions
 * ───────────────────────────────────────────────────────────── */

static void check_durio_enemy(Game *g) {
    Rect mr = durio_hitbox(&g->durio);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (e->type == ENEMY_NONE || e->status == ENEMY_DEAD) continue;
        if (e->status == ENEMY_SQUISHED) continue;

        Rect er = enemy_hitbox(e);
        if (!rect_overlap(&mr, &er)) continue;

        int durio_foot = FP_TO_INT(g->durio.y_fp) + durio_height(&g->durio);
        int enemy_top  = FP_TO_INT(e->y_fp);

        if (durio_foot <= enemy_top + 8 && g->durio.vy_fp > 0) {
            /* Stomp */
            g->durio.vy_fp = INT_TO_FP(-260);
            g->durio.score += (e->type == ENEMY_CRAB) ? SCORE_CRAB : SCORE_SNAIL;

            if (e->type == ENEMY_CRAB) {
                e->status       = ENEMY_SQUISHED;
                e->squish_timer = 300;
            } else {
                int shell_tx = FP_TO_INT(e->x_fp) / TILE_PX;
                int shell_ty = FP_TO_INT(e->y_fp) / TILE_PX;
                level_set_tile(&g->level, shell_tx, shell_ty, TILE_SHELL);
                e->status = ENEMY_DEAD;
            }
        } else {
            durio_kill(&g->durio);
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Durio <-> flying-nut collisions
 * ───────────────────────────────────────────────────────────── */

static void check_durio_nuts(Game *g) {
    Rect mr = durio_hitbox(&g->durio);

    for (int i = 0; i < MAX_NUTS; i++) {
        Nut *c = &g->nuts[i];
        if (!c->active) continue;

        Rect cr = nut_hitbox(c);
        if (rect_overlap(&mr, &cr)) {
            c->active = 0;
            g->durio.nuts++;
            g->durio.score += SCORE_NUT;
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Durio walks through a TILE_NUT in the map
 * ───────────────────────────────────────────────────────────── */

static void check_durio_tile_nuts(Game *g) {
    Rect mr = durio_hitbox(&g->durio);
    int tx0 = mr.x / TILE_PX;
    int tx1 = (mr.x + mr.w - 1) / TILE_PX;
    int ty0 = mr.y / TILE_PX;
    int ty1 = (mr.y + mr.h - 1) / TILE_PX;
    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (level_tile(&g->level, tx, ty) == TILE_NUT) {
                level_set_tile(&g->level, tx, ty, TILE_AIR);
                g->durio.nuts++;
                g->durio.score += SCORE_NUT;
            }
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Spawn a nut from a popped ?-block
 * ───────────────────────────────────────────────────────────── */

static void spawn_nut_from_block(Game *g, int tx, int ty) {
    for (int i = 0; i < MAX_NUTS; i++) {
        if (!g->nuts[i].active) {
            nut_spawn(&g->nuts[i],
                       tx * TILE_PX + 8,
                       ty * TILE_PX - TILE_PX,
                       1);
            return;
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Main tick
 * ───────────────────────────────────────────────────────────── */

void game_tick(Game *g, int delta_ms) {
    switch (g->state) {
    case GAME_TITLE:
        return;

    case GAME_PAUSED:
        return;

    case GAME_DEAD:
        durio_update(&g->durio, &g->level,
                     0, 0, 0, 0, 0, delta_ms);
        g->dead_wait += delta_ms;
        return;

    case GAME_PLAYING:
        break;
    }

    /* ── Block advance check ── */
    check_block_advance(g);

    /* ── Update enemies ── */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies[i].type == ENEMY_NONE) continue;
        enemy_update(&g->enemies[i], &g->level, delta_ms);
    }

    /* ── Update nuts ── */
    for (int i = 0; i < MAX_NUTS; i++)
        nut_update(&g->nuts[i], delta_ms);

    /* ── Collisions ── */
    if (g->durio.state != DURIO_DEAD) {
        check_durio_enemy(g);
        check_durio_nuts(g);
        check_durio_tile_nuts(g);
    }

    /* ── Distance score: +SCORE_TILE per new tile reached ── */
    {
        int cur_tx = FP_TO_INT(g->durio.x_fp) / TILE_PX;
        if (cur_tx > g->durio.max_tile_x) {
            g->durio.score += (cur_tx - g->durio.max_tile_x) * SCORE_TILE;
            g->durio.max_tile_x = cur_tx;
        }
    }

    /* ── Camera + back-scroll ── */
    update_camera(g);

    /* ── Death transition ── */
    if (g->durio.state == DURIO_DEAD) {
        if (g->durio.lives > 0) {
            game_respawn(g);
        } else {
            if (g->durio.score > g->save.high_score) {
                g->save.high_score = g->durio.score;
                save_write(&g->save);
            }
            g->state = GAME_DEAD;
        }
    }
}

void game_input(Game *g, int left, int right, int run,
                int jump_down, int jump_held,
                int pause, int confirm, int back, int delta_ms) {
    switch (g->state) {
    case GAME_TITLE:
        if (confirm || pause) game_start(g);
        return;

    case GAME_PLAYING:
        if (pause) { g->state = GAME_PAUSED; return; }
        durio_update(&g->durio, &g->level,
                     left, right, run, jump_down, jump_held, delta_ms);
        if (g->durio.pending_coin_tx >= 0) {
            spawn_nut_from_block(g, g->durio.pending_coin_tx,
                                     g->durio.pending_coin_ty);
            g->durio.pending_coin_tx = -1;
        }
        return;

    case GAME_PAUSED:
        if (pause || confirm) g->state = GAME_PLAYING;
        if (back) { g->state = GAME_TITLE; game_init(g); }
        return;

    case GAME_DEAD:
        if (g->dead_wait < DEAD_WAIT_MS) return;
        if (confirm || back) game_init(g);
        return;
    }
}
