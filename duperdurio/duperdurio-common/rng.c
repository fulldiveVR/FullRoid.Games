#include "rng.h"
#include <stdlib.h>

void rng_seed(unsigned int seed) {
    srand(seed);
}

int rng_next(int max) {
    if (max <= 0) return 0;
    return rand() % max;
}
