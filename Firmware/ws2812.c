#include "ws2812.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/cortex.h>
#include <stdint.h>


#define NOP1  __asm__("nop")
#define NOP4  NOP1;NOP1;NOP1;NOP1
#define NOP8  NOP4;NOP4
#define NOP16 NOP8;NOP8


#define DELAY_T0H()  NOP16; NOP1; NOP1; NOP1          /* ~19 cycles / 395ns */
#define DELAY_T0L()  NOP16; NOP16; NOP8; NOP1          /* ~41 cycles / 854ns */
#define DELAY_T1H()  NOP16; NOP16; NOP4; NOP1; NOP1   /* ~38 cycles / 791ns */
#define DELAY_T1L()  NOP16; NOP4; NOP1; NOP1           /* ~22 cycles / 458ns */


static inline void __attribute__((optimize("O2")))
send_byte(volatile uint32_t *bsrr, uint32_t set_mask, uint32_t clr_mask,
          uint8_t byte)
{
    for (int bit = 7; bit >= 0; bit--) {
        if (byte & (1 << bit)) {
            *bsrr = set_mask;   DELAY_T1H();
            *bsrr = clr_mask;   DELAY_T1L();
        } else {
            *bsrr = set_mask;   DELAY_T0H();
            *bsrr = clr_mask;   DELAY_T0L();
        }
    }
}


void ws2812_send(uint32_t port, uint16_t pin,
                 const uint32_t *pixels, uint32_t count)
{
    volatile uint32_t *bsrr = (volatile uint32_t *)(port + 0x18U); 
    uint32_t set_mask = (uint32_t)pin;
    uint32_t clr_mask = (uint32_t)pin << 16;

    cm_disable_interrupts();

    for (uint32_t i = 0; i < count; i++) {
        uint8_t r = (pixels[i] >> 16) & 0xFF;
        uint8_t g = (pixels[i] >>  8) & 0xFF;
        uint8_t b = (pixels[i]      ) & 0xFF;

        send_byte(bsrr, set_mask, clr_mask, g);
        send_byte(bsrr, set_mask, clr_mask, r);
        send_byte(bsrr, set_mask, clr_mask, b);
    }
    *bsrr = clr_mask;
    for (volatile int i = 0; i < 2500; i++) __asm__("nop");

    cm_enable_interrupts();
}
