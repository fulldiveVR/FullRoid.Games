#include "../../numberslide-common/save.h"
#include <nds.h>
#include <string.h>

/*
 * NDS SRAM layout at offset 0:
 *   [0..3]  magic  (SAVE_MAGIC)
 *   [4..7]  best_score (little-endian int)
 */

#define SRAM_BASE  ((volatile uint8_t *)0x0A000000)

static void sram_read(int offset, void *dst, int len) {
    volatile uint8_t *src = SRAM_BASE + offset;
    uint8_t *d = (uint8_t *)dst;
    for (int i = 0; i < len; i++)
        d[i] = src[i];
}

static void sram_write_bytes(int offset, const void *src, int len) {
    volatile uint8_t *dst = SRAM_BASE + offset;
    const uint8_t *s = (const uint8_t *)src;
    for (int i = 0; i < len; i++)
        dst[i] = s[i];
}

void save_load(int *best_score) {
    *best_score = 0;

    unsigned int magic = 0;
    sram_read(0, &magic, 4);
    if (magic != SAVE_MAGIC) return;

    int val = 0;
    sram_read(4, &val, 4);
    if (val > 0 && val < 10000000)
        *best_score = val;
}

void save_write(int best_score) {
    unsigned int magic = SAVE_MAGIC;
    sram_write_bytes(0, &magic, 4);
    sram_write_bytes(4, &best_score, 4);
}
