#pragma once

/* Platform must call rng_seed() at startup with a hardware source:
 *   NDS: TIMER0_DATA | (TIMER1_DATA << 16)
 *   3DS: osGetTime() or svcGetSystemTick()
 */
void rng_seed(unsigned int seed);

/* Returns a random integer in [0, max) */
int  rng_next(int max);
