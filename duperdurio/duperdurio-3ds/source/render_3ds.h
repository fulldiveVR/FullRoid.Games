#pragma once
#include "../../duperdurio-common/game.h"
#include <3ds.h>
#include <citro2d.h>

void render_init_3ds(void);
void render_exit_3ds(void);
void render_frame_3ds(const Game *g, C3D_RenderTarget *top, C3D_RenderTarget *bot);
