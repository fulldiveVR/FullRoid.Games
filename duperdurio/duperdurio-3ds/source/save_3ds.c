#include "../../duperdurio-common/save.h"
#include <3ds.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#define SAVE_DIR   "/3ds/duperdurio"
#define SAVE_PATH  SAVE_DIR "/save.dat"

void save_load(SaveData *out) {
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return;

    SaveData tmp;
    if (fread(&tmp, sizeof(tmp), 1, f) == 1 && tmp.magic == SAVE_MAGIC)
        *out = tmp;

    fclose(f);
}

void save_write(const SaveData *data) {
    mkdir(SAVE_DIR, 0755);

    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f) return;

    SaveData tmp = *data;
    tmp.magic = SAVE_MAGIC;
    fwrite(&tmp, sizeof(tmp), 1, f);
    fclose(f);
}
