#pragma once
#include "../../snake-common/game.h"
#include "../../snake-common/i18n/i18n.h"
#include <citro2d.h>

void render_init_3ds(void);
void render_frame_3ds(const Game *g,
                      C3D_RenderTarget *top,
                      C3D_RenderTarget *bot);
void render_exit_3ds(void);

Language lang_detect_system(void);
