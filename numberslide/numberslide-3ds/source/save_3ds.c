#include "../../numberslide-common/save.h"
#include <3ds.h>
#include <stdio.h>
#include <sys/stat.h>

#define SAVE_DIR   "/3ds/numberslide"
#define SAVE_PATH  SAVE_DIR "/save.dat"

typedef struct {
    unsigned int magic;
    int best_score;
} SaveData;

void save_load(int *best_score) {
    *best_score = 0;

    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return;

    SaveData data;
    if (fread(&data, sizeof(data), 1, f) == 1) {
        if (data.magic == SAVE_MAGIC && data.best_score > 0 && data.best_score < 10000000)
            *best_score = data.best_score;
    }
    fclose(f);
}

void save_write(int best_score) {
    mkdir(SAVE_DIR, 0755);

    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f) return;

    SaveData data;
    data.magic = SAVE_MAGIC;
    data.best_score = best_score;
    fwrite(&data, sizeof(data), 1, f);
    fclose(f);
}
