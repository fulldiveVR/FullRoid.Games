/*
 * Compatibility stubs for building with old ds_arm9_crt0 + new libnds 2.x.
 * Bridges old crt0 expectations AND calico runtime functions.
 */
#include <nds.h>
#include <calico/system/thread.h>
#include <calico/nds/touch.h>

/* ================================================================
 * Stubs for OLD ds_arm9_crt0
 * ================================================================ */

void __libnds_mpu_setup(void) {}

void initSystem(void) {
    REG_POWCNT = POWCNT_LCD | POWCNT_2D_GFX_A | POWCNT_2D_GFX_B | POWCNT_LCD_SWAP;
    REG_IME = 1;
}

void __libnds_exit(void) {
    for (;;) asm volatile("swi 0x06");
}

int __dsimode = 0;
unsigned int __secure_area__ = 0;

/* ================================================================
 * Calico runtime stubs — match exact signatures from headers
 * ================================================================ */

u32 threadIrqWait(bool next_irq, IrqMask mask) {
    register u32 r0 asm("r0") = next_irq ? 1 : 0;
    register u32 r1 asm("r1") = (u32)mask;
    asm volatile("swi 0x04" : "+r"(r0), "+r"(r1) :: "r2", "r3", "memory");
    return r0;
}

unsigned keypadGetState(void) {
    unsigned state = ~(*(volatile unsigned short *)0x04000130) & 0x03FF;
    unsigned ext = ~(*(volatile unsigned short *)0x027FF000);
    if (ext & (1 << 0)) state |= (1 << 10); /* KEY_X */
    if (ext & (1 << 1)) state |= (1 << 11); /* KEY_Y */
    if (ext & (1 << 6)) state |= (1 << 14); /* KEY_TOUCH (pen down) */
    if (ext & (1 << 7)) state |= (1 << 15); /* KEY_LID */
    return state;
}

bool touchRead(TouchData *out) {
    if (out) {
        out->rawx = 0;
        out->rawy = 0;
        out->px = 0;
        out->py = 0;
    }
    return false;
}
