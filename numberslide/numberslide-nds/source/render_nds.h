#pragma once
#include "../../numberslide-common/game.h"
#include "../../numberslide-common/i18n/i18n.h"

void render_init(void);
void render_swap(void);
void render_top(const Game *g);
void render_bottom(const Game *g);

Language lang_detect_system(void);
