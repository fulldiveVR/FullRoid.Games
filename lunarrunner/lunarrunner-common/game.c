#include "game.h"
#include "save.h"
#include "rng.h"
#include "i18n/i18n.h"

/* --- AABB collision --- */

static int aabb_overlap(int ax1, int ay1, int ax2, int ay2,
                        int bx1, int by1, int bx2, int by2) {
    return ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1;
}

/* --- Obstacle spawning logic --- */

static void try_spawn_obstacle(Game *g) {
    int speed_px = FP_TO_INT(g->world.scroll_speed_fp);
    if (speed_px <= 0) return;
    int gap_px = OBSTACLE_MIN_GAP_MS * speed_px / 1000;
    if (gap_px < 40) gap_px = 40;

    if (g->world.spawn_distance < gap_px) return;

    int dist = g->world.distance;
    ObstacleType type;
    int variant = 0;

    /* Combo pending: spawn antenna (only from explicit combo trigger) */
    if (g->combo_pending) {
        g->world.spawn_distance = 0;
        g->combo_pending = 0;
        g->last_obstacle = OBS_ANTENNA;
        obstacle_spawn(g->obstacles, OBS_ANTENNA, 0);
        return;
    }

    /* Roll obstacle type once */
    if (dist < 200) {
        type = rng_next(2) == 0 ? OBS_CRATER : OBS_BOULDER;
    } else if (dist < COMBO_DISTANCE) {
        int r = rng_next(4);
        if (r == 0)      type = OBS_CRATER;
        else if (r == 1) type = OBS_BOULDER;
        else if (r == 2) type = OBS_ANTENNA;
        else {
            type = OBS_ALIEN_FLOWER;
            variant = rng_next(2);
        }
    } else {
        /* Meteor rain chance */
        if (!g->meteor.active && rng_next(20) == 0) {
            g->meteor.active        = 1;
            g->meteor.warning_timer = 1000;
            g->meteor.rain_timer    = 0;
            return;
        }
        /* Combo chance (only if last was NOT antenna) */
        if (g->last_obstacle != OBS_ANTENNA && rng_next(5) == 0) {
            g->world.spawn_distance = 0;
            g->last_obstacle = OBS_CRATER;
            obstacle_spawn(g->obstacles, OBS_CRATER, 0);
            g->combo_pending = 1;
            g->world.spawn_distance = gap_px * 6 / 10;
            return;
        }
        int r = rng_next(4);
        if (r == 0)      type = OBS_CRATER;
        else if (r == 1) type = OBS_BOULDER;
        else if (r == 2) type = OBS_ANTENNA;
        else {
            type = OBS_ALIEN_FLOWER;
            variant = rng_next(2);
        }
    }

    /* Antenna after a non-antenna needs double gap (time to land + duck).
       If not enough distance yet, skip this spawn entirely. */
    if (type == OBS_ANTENNA && g->last_obstacle != OBS_ANTENNA) {
        int extra_gap = gap_px * 2;
        if (g->world.spawn_distance < extra_gap) return;
    }

    g->world.spawn_distance = 0;
    g->last_obstacle = type;
    obstacle_spawn(g->obstacles, type, variant);
}

/* --- Collectible spawning --- */

static void try_spawn_collectible(Game *g) {
    /* Spawn crystals with some randomness — ~1 per 6 frames at 60fps */
    if (rng_next(6) != 0) return;

    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        if (g->collectibles[i].active) continue;
        Collectible *c = &g->collectibles[i];
        c->active = 1;
        c->x_fp   = INT_TO_FP(SCREEN_W + 8);

        /* Shield and stardust are rare */
        int r = rng_next(100);
        if (r < 2) {
            c->type = COLLECT_SHIELD;
            c->y    = GROUND_Y - ROVER_H;
        } else if (r < 7) {
            c->type = COLLECT_STARDUST;
            c->y    = GROUND_Y - ROVER_H + 2 - rng_next(16);
        } else {
            c->type = COLLECT_CRYSTAL;
            /* Most at running height, some higher (jump to collect) */
            c->y    = GROUND_Y - ROVER_H + 4 - rng_next(20);
        }
        break;
    }
}

/* --- Collision detection --- */

static void check_collisions(Game *g) {
    /* Rover hitbox — significantly smaller than sprite.
       NDS example: sprite 24×16, hitbox 12×10 (centered). */
    int ry = rover_bottom(&g->rover) - ROVER_HIT_INSET_BOT;
    int rt = rover_top(&g->rover) + ROVER_HIT_INSET_TOP;
    int rx1 = g->rover.x + ROVER_HIT_INSET_X;
    int rx2 = g->rover.x + ROVER_W - ROVER_HIT_INSET_X;

    /* Obstacles */
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        Obstacle *o = &g->obstacles[i];
        if (!o->active) continue;

        int ox = FP_TO_INT(o->x_fp);
        if (aabb_overlap(rx1, rt, rx2, ry,
                         ox, o->y, ox + o->w, o->y + o->h)) {
            if (bonus_is_turbo(&g->bonus)) continue;
            if (g->rover.has_shield) {
                g->rover.has_shield = 0;
                o->active = 0;
                fx_trigger(&g->fx, FX_SHIELD_BREAK, ox, o->y);
                continue;
            }
            fx_trigger(&g->fx, FX_DEATH, rx1, rt);
            g->state = STATE_GAME_OVER;
            return;
        }
    }

    /* Meteor rain */
    if (g->meteor.active && g->meteor.rain_timer > 0) {
        int ducking = (g->rover.action == ROVER_DUCK ||
                       g->rover.action == ROVER_JUMP_DUCK);
        if (!ducking) {
            if (bonus_is_turbo(&g->bonus)) { /* safe */ }
            else if (g->rover.has_shield) {
                g->rover.has_shield = 0;
                fx_trigger(&g->fx, FX_SHIELD_BREAK, rx1, rt);
            } else {
                fx_trigger(&g->fx, FX_DEATH, rx1, rt);
                g->state = STATE_GAME_OVER;
                return;
            }
        }
    }

    /* Collectibles */
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        Collectible *c = &g->collectibles[i];
        if (!c->active) continue;

        int cx = FP_TO_INT(c->x_fp);
        int cw = 8, ch = 8;
        if (!aabb_overlap(rx1, rt, rx2, ry,
                          cx, c->y, cx + cw, c->y + ch))
            continue;

        c->active = 0;
        fx_trigger(&g->fx, FX_COLLECT_FLASH, cx, c->y);

        switch (c->type) {
        case COLLECT_CRYSTAL:
            g->score += CRYSTAL_SCORE * bonus_score_multiplier(&g->bonus);
            bonus_add_bar(&g->bonus, 5);
            break;
        case COLLECT_STARDUST:
            g->score += STARDUST_SCORE * bonus_score_multiplier(&g->bonus);
            bonus_add_bar(&g->bonus, STARDUST_BAR);
            break;
        case COLLECT_SHIELD:
            g->rover.has_shield = 1;
            break;
        }
    }
}

/* --- Public API --- */

void game_init(Game *g) {
    g->state         = STATE_MENU;
    g->score         = 0;
    g->best_score    = 0;
    g->combo_pending = 0;
    g->last_obstacle = OBS_CRATER;
    rover_init(&g->rover);
    world_init(&g->world);
    bonus_init(&g->bonus);
    obstacle_init_all(g->obstacles, &g->meteor);
    collectible_init_all(g->collectibles);
    fx_init(&g->fx);
}

void game_start(Game *g) {
    g->state         = STATE_PLAYING;
    g->score         = 0;
    g->combo_pending = 0;
    g->last_obstacle = OBS_CRATER;
    rover_init(&g->rover);
    world_init(&g->world);
    bonus_init(&g->bonus);
    obstacle_init_all(g->obstacles, &g->meteor);
    collectible_init_all(g->collectibles);
    fx_init(&g->fx);
}

void game_tick(Game *g, int delta_ms) {
    if (g->state != STATE_PLAYING) return;

    /* World scroll */
    int dx_px = world_tick(&g->world, delta_ms);

    /* Distance-based score (clamped to prevent int overflow) */
    int mult = bonus_score_multiplier(&g->bonus);
    int score_add = FP_TO_INT(g->world.scroll_speed_fp) * delta_ms * mult / 10000;
    g->score += score_add;
    if (g->score > 9999999) g->score = 9999999;

    /* Rover physics */
    rover_tick(&g->rover, delta_ms);

    /* Move objects */
    obstacle_move_all(g->obstacles, dx_px);
    collectible_move_all(g->collectibles, dx_px);
    meteor_tick(&g->meteor, delta_ms);

    /* Magnet pull — uses integer distance, pulls in fixed-point */
    if (bonus_is_magnet(&g->bonus)) {
        int ry = rover_bottom(&g->rover);
        int rover_x_fp = INT_TO_FP(g->rover.x);
        for (int i = 0; i < MAX_COLLECTIBLES; i++) {
            Collectible *c = &g->collectibles[i];
            if (!c->active) continue;
            int cx = FP_TO_INT(c->x_fp);
            int dx = g->rover.x - cx;
            int dy = ry - c->y;
            if (dx * dx + dy * dy < MAGNET_RADIUS * MAGNET_RADIUS) {
                /* Pull X in fixed-point towards rover */
                int pull_x_fp = (rover_x_fp - c->x_fp) / 4;
                c->x_fp += pull_x_fp;
                /* Pull Y in integer */
                if (dy > 2) c->y += 2;
                else if (dy < -2) c->y -= 2;
            }
        }
    }

    /* Spawn */
    try_spawn_obstacle(g);
    try_spawn_collectible(g);

    /* Collision */
    check_collisions(g);

    /* Bonuses */
    bonus_tick(&g->bonus, delta_ms);
    fx_tick(&g->fx, delta_ms);

    /* Turbo speed lines effect */
    if (bonus_is_turbo(&g->bonus) && g->fx.active_fx != FX_TURBO_LINES)
        fx_trigger(&g->fx, FX_TURBO_LINES, 0, 0);

    /* Best score */
    if (g->score > g->best_score) {
        g->best_score = g->score;
        save_write(g->best_score);
    }
}

void game_input(Game *g, const InputState *input) {
    switch (g->state) {
    case STATE_MENU:
        if (input->confirm)
            game_start(g);
        break;

    case STATE_PLAYING:
        /* Jump */
        if (input->jump && g->rover.action == ROVER_RUN) {
            g->rover.vy_fp  = JUMP_VELOCITY;
            g->rover.action = ROVER_JUMP;
        }
        /* Duck on ground */
        if (input->duck && g->rover.action == ROVER_RUN)
            g->rover.action = ROVER_DUCK;
        if (input->duck_released && g->rover.action == ROVER_DUCK)
            g->rover.action = ROVER_RUN;
        /* Duck in air */
        if (input->duck && g->rover.action == ROVER_JUMP)
            g->rover.action = ROVER_JUMP_DUCK;
        if (input->duck_released && g->rover.action == ROVER_JUMP_DUCK)
            g->rover.action = ROVER_JUMP;
        /* Bonus */
        if (input->bonus)
            bonus_activate(&g->bonus);
        /* Pause */
        if (input->pause)
            g->state = STATE_PAUSED;
        break;

    case STATE_PAUSED:
        if (input->pause || input->confirm)
            g->state = STATE_PLAYING;
        if (input->back)
            g->state = STATE_MENU;
        break;

    case STATE_GAME_OVER:
        if (input->confirm)
            game_start(g);
        if (input->back)
            g->state = STATE_MENU;
        break;
    }

    /* Language switching — L/R always active */
    if (input->lang_next)
        lang_set((lang_get_current() + 1) % LANG_COUNT);
    if (input->lang_prev)
        lang_set((lang_get_current() + LANG_COUNT - 1) % LANG_COUNT);
}
