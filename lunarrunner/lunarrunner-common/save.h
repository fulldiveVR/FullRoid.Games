#pragma once

#define SAVE_MAGIC  0x4C524E52  /* "LRNR" */

void save_load(int *best_score);
void save_write(int best_score);
