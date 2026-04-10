#pragma once
#include <stdint.h>

#define SAVE_MAGIC  0x44555052  /* "DUPR" */

typedef struct {
    uint32_t magic;
    int      high_score;
    uint8_t  best_world;
    uint8_t  best_level;
} SaveData;

void save_load(SaveData *out);
void save_write(const SaveData *data);
