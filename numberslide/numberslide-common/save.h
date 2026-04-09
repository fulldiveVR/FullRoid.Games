#pragma once

/*
 * Platform-specific save/load for best score.
 * Implemented in:
 *   NDS: save_nds.c  (SRAM at fixed offset)
 *   3DS: save_3ds.c  (file on SD card)
 */

#define SAVE_MAGIC  0x4E534C44  /* "NSLD" */

void save_load(int *best_score);
void save_write(int best_score);
