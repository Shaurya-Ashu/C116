

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "ws2812.h"
#include "vl53l0x.h"
#include "usb_cdc.h"


static volatile uint32_t ms_ticks = 0;

void sys_tick_handler(void) { ms_ticks++; }

static void delay_ms(uint32_t ms)
{
    uint32_t end = ms_ticks + ms;
    while (ms_ticks < end) __asm__("nop");
}

static uint32_t millis(void) { return ms_ticks; }


static void clock_setup(void)
{
   
    RCC_CR2 |= RCC_CR2_HSI48ON;
    while (!(RCC_CR2 & RCC_CR2_HSI48RDY));

    
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI48;
    while ((RCC_CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI48);

    
    rcc_periph_clock_enable(RCC_CRS);
    crs_autotrim_usb_enable();

   
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(48000 - 1);  
    systick_interrupt_enable();
    systick_counter_enable();
}


static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOF);

    
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO5);

    
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    GPIO0 | GPIO3 | GPIO7);

    
    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO8);
}


static void usart_setup(void)
{
    rcc_periph_clock_enable(RCC_USART1);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
    gpio_set_af(GPIOA, GPIO_AF1, GPIO9 | GPIO10);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);
}


static void i2c_setup(void)
{
    rcc_periph_clock_enable(RCC_I2C1);

    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
    gpio_set_af(GPIOB, GPIO_AF1, GPIO6 | GPIO7);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH,
                            GPIO6 | GPIO7);
    i2c_peripheral_disable(I2C1);
    I2C1_TIMINGR = (0xB << 28) | (4 << 20) | (2 << 16) | (15 << 8) | 19;
    i2c_peripheral_enable(I2C1);
}


static void uart_send_str(const char *s)
{
    while (*s) {
        usart_send_blocking(USART1, (uint8_t)*s++);
    }
}


static void jump_to_dfu(void)
{
    
    const uint32_t BOOTLOADER_ADDR = 0x1FFFC800UL;
    usb_cdc_send("[BOOT0] Jumping to DFU bootloader...\r\n");
    delay_ms(100);

    systick_counter_disable();
    __asm__("cpsid i");

   
    SYSCFG_CFGR1 = (SYSCFG_CFGR1 & ~3) | 1;

    
    uint32_t sp  = *(volatile uint32_t *)(BOOTLOADER_ADDR);
    uint32_t pc  = *(volatile uint32_t *)(BOOTLOADER_ADDR + 4);
    __asm__ volatile (
        "msr msp, %0   \n"
        "bx  %1        \n"
        : : "r"(sp), "r"(pc)
    );
}


static void test_leds(void)
{
    usb_cdc_send("[LED]  Starting WS2812B sweep...\r\n");

   
    uint32_t colors[] = {
        0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF, 0x000000
    };
    const char *names[] = { "RED", "GREEN", "BLUE", "WHITE", "OFF" };

    for (int c = 0; c < 5; c++) {
        char buf[48];
        snprintf(buf, sizeof(buf), "[LED]  All 4 → %s\r\n", names[c]);
        usb_cdc_send(buf);

        
        uint32_t frame[4] = {
            colors[c], colors[c], colors[c], colors[c]
        };
        ws2812_send(GPIOA, GPIO5, frame, 4);
        delay_ms(400);
    }

    
    usb_cdc_send("[LED]  Chase pattern...\r\n");
    for (int i = 0; i < 4; i++) {
        uint32_t frame[4] = {0, 0, 0, 0};
        frame[i] = 0x003300;  
        ws2812_send(GPIOA, GPIO5, frame, 4);
        delay_ms(200);
    }


    uint32_t off[4] = {0,0,0,0};
    ws2812_send(GPIOA, GPIO5, off, 4);
    usb_cdc_send("[LED]  Sweep complete ✓\r\n");
}


static void test_uart(void)
{
    usb_cdc_send("[UART] Sending PING on USART1 TX (PA9)...\r\n");
    uart_send_str("PING\r\n");

    uint32_t t0 = millis();
    char rx_buf[8];
    int  rx_idx = 0;
    while (millis() - t0 < 50 && rx_idx < 6) {
        if (usart_get_flag(USART1, USART_ISR_RXNE)) {
            rx_buf[rx_idx++] = (char)usart_recv(USART1);
        }
    }
    rx_buf[rx_idx] = '\0';

    if (rx_idx >= 4 && rx_buf[0] == 'P') {
        usb_cdc_send("[UART] RX loopback OK ✓ (got: PING)\r\n");
    } else {
        usb_cdc_send("[UART] RX no echo — wire PA9→PA10 for loopback test\r\n");
    }
}


static void test_tof(void)
{
    usb_cdc_send("[TOF]  Initialising VL53L0X on I2C1...\r\n");

    if (!vl53l0x_init(I2C1)) {
        usb_cdc_send("[TOF]  FAIL — sensor not found (check XSHUT tied HIGH)\r\n");
        return;
    }

    usb_cdc_send("[TOF]  Sensor ID OK ✓\r\n");
    usb_cdc_send("[TOF]  Taking 5 range readings:\r\n");

    for (int i = 0; i < 5; i++) {
        uint16_t dist_mm = vl53l0x_read_range_mm(I2C1);
        char buf[48];
        if (dist_mm == 0xFFFF) {
            snprintf(buf, sizeof(buf), "[TOF]    #%d  OUT OF RANGE\r\n", i+1);
        } else {
            snprintf(buf, sizeof(buf), "[TOF]    #%d  %4u mm\r\n", i+1, dist_mm);
        }
        usb_cdc_send(buf);
        delay_ms(100);
    }
}

static void test_gpio(void)
{
    usb_cdc_send("[GPIO] Toggling GPIO1(PA0), GPIO2(PA3), GPIO3(PA7) x3...\r\n");

    uint32_t pins[] = { GPIO0, GPIO3, GPIO7 };
    const char *names[] = { "GPIO1/PA0", "GPIO2/PA3", "GPIO3/PA7" };

    for (int rep = 0; rep < 3; rep++) {
        for (int i = 0; i < 3; i++) {
            char buf[40];
            snprintf(buf, sizeof(buf), "[GPIO] %s HIGH\r\n", names[i]);
            usb_cdc_send(buf);
            gpio_set(GPIOA, pins[i]);
            delay_ms(150);
            gpio_clear(GPIOA, pins[i]);
            delay_ms(100);
        }
    }
    usb_cdc_send("[GPIO] GPIO test complete ✓\r\n");
}


int main(void)
{
    clock_setup();
    gpio_setup();
    usart_setup();
    i2c_setup();
    usbd_device *usb_dev = usb_cdc_init();

   
    uint32_t t0 = millis();
    while (millis() - t0 < 2000) {
        usbd_poll(usb_dev);
    }

    usb_cdc_send("\r\n");
    usb_cdc_send("============================================\r\n");
    usb_cdc_send("  Tiny MCU C116 — Board Test Firmware\r\n");
    usb_cdc_send("  STM32F042T6Y6TR\r\n");
    usb_cdc_send("============================================\r\n\r\n");

    /* ── Run all tests once ── */
    test_leds();    delay_ms(500);
    test_uart();    delay_ms(200);
    test_tof();     delay_ms(200);
    test_gpio();    delay_ms(200);

    usb_cdc_send("\r\n[DONE] All tests complete. Entering live mode.\r\n");
    usb_cdc_send("[DONE] ToF streaming every 250ms. Press BOOT button for DFU.\r\n\r\n");

    /* ── Live loop ── */
    uint32_t last_tof  = millis();
    uint32_t last_led  = millis();
    uint8_t  led_phase = 0;

    while (1) {
        usbd_poll(usb_dev);

        
        if (gpio_get(GPIOB, GPIO8)) {
            delay_ms(50);                    
            if (gpio_get(GPIOB, GPIO8)) {
                jump_to_dfu();
            }
        }

        
        if (millis() - last_tof >= 250) {
            last_tof = millis();
            uint16_t d = vl53l0x_read_range_mm(I2C1);
            char buf[32];
            if (d == 0xFFFF)
                usb_cdc_send("[TOF] OUT OF RANGE\r\n");
            else {
                snprintf(buf, sizeof(buf), "[TOF] %4u mm\r\n", d);
                usb_cdc_send(buf);
            }

            
            uint32_t col;
            if      (d == 0xFFFF) col = 0x000022;   
            else if (d < 200)     col = 0x220000;   
            else if (d < 500)     col = 0x221100;   
            else                  col = 0x002200;   

            uint32_t frame[4] = {col, col, col, col};
            ws2812_send(GPIOA, GPIO5, frame, 4);
        }

        
        if (millis() - last_led >= 500) {
            last_led = millis();
            led_phase ^= 1;
            if (led_phase) gpio_set(GPIOA, GPIO0);
            else           gpio_clear(GPIOA, GPIO0);
        }

        
        if (usart_get_flag(USART1, USART_ISR_RXNE)) {
            char ch = (char)usart_recv(USART1);
            usart_send_blocking(USART1, (uint8_t)ch);   
            char buf[4] = {ch, 0};
            usb_cdc_send(buf);                           
        }
    }

    return 0;
}
