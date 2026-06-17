/**
 * ws2812.c — Bit-bang WS2812B driver for STM32F042 @ 48 MHz
 *
 * Cycle counts at 48 MHz (1 cycle ≈ 20.8 ns):
 *   T0H = 400 ns →  19 cycles (NOP loop)
 *   T0L = 850 ns →  41 cycles
 *   T1H = 800 ns →  38 cycles
 *   T1L = 450 ns →  22 cycles
 *   RES > 50 µs  → 2400 cycles
 *
 * GPIO BSRR (bit set/reset register) used for atomic pin control.
 * All timing via NOP sequences — no timer needed.
 */

#include "ws2812.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/cortex.h>
#include <stdint.h>

/* ── NOP helpers ─────────────────────────────────────────────────────────── */
#define NOP1  __asm__("nop")
#define NOP4  NOP1;NOP1;NOP1;NOP1
#define NOP8  NOP4;NOP4
#define NOP16 NOP8;NOP8

/* Precise delays at 48 MHz */
#define DELAY_T0H()  NOP16; NOP1; NOP1; NOP1          /* ~19 cycles / 395ns */
#define DELAY_T0L()  NOP16; NOP16; NOP8; NOP1          /* ~41 cycles / 854ns */
#define DELAY_T1H()  NOP16; NOP16; NOP4; NOP1; NOP1   /* ~38 cycles / 791ns */
#define DELAY_T1L()  NOP16; NOP4; NOP1; NOP1           /* ~22 cycles / 458ns */

/* ── Internal: send one byte (MSB first) ────────────────────────────────── */
static inline void __attribute__((optimize("O2")))
send_byte(volatile uint32_t *bsrr, uint32_t set_mask, uint32_t clr_mask,
          uint8_t byte)
{
    for (int bit = 7; bit >= 0; bit--) {
        if (byte & (1 << bit)) {
            /* Send 1 */
            *bsrr = set_mask;   DELAY_T1H();
            *bsrr = clr_mask;   DELAY_T1L();
        } else {
            /* Send 0 */
            *bsrr = set_mask;   DELAY_T0H();
            *bsrr = clr_mask;   DELAY_T0L();
        }
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void ws2812_send(uint32_t port, uint16_t pin,
                 const uint32_t *pixels, uint32_t count)
{
    /* BSRR: upper 16 bits = reset (clear), lower 16 bits = set */
    volatile uint32_t *bsrr = (volatile uint32_t *)(port + 0x18U); /* GPIO_BSRR */
    uint32_t set_mask = (uint32_t)pin;
    uint32_t clr_mask = (uint32_t)pin << 16;

    cm_disable_interrupts();

    for (uint32_t i = 0; i < count; i++) {
        /* Convert 0x00RRGGBB → WS2812B GRB order */
        uint8_t r = (pixels[i] >> 16) & 0xFF;
        uint8_t g = (pixels[i] >>  8) & 0xFF;
        uint8_t b = (pixels[i]      ) & 0xFF;

        send_byte(bsrr, set_mask, clr_mask, g);
        send_byte(bsrr, set_mask, clr_mask, r);
        send_byte(bsrr, set_mask, clr_mask, b);
    }

    /* Reset pulse: pin LOW > 50 µs */
    *bsrr = clr_mask;
    for (volatile int i = 0; i < 2500; i++) __asm__("nop");

    cm_enable_interrupts();
}
