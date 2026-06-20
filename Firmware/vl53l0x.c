#include "vl53l0x.h"
#include <libopencm3/stm32/i2c.h>
#include <stdint.h>
#include <stdbool.h>


#define I2C_TIMEOUT_MS 10

static volatile uint32_t *_ms; 
static void reg_write(uint32_t i2c, uint8_t reg, uint8_t val)
{
    i2c_transfer7(i2c, VL53L0X_ADDR, (uint8_t[]){reg, val}, 2, NULL, 0);
}


static uint8_t reg_read(uint32_t i2c, uint8_t reg)
{
    uint8_t data = 0;
    i2c_transfer7(i2c, VL53L0X_ADDR, &reg, 1, &data, 1);
    return data;
}


static uint16_t reg_read16(uint32_t i2c, uint8_t reg)
{
    uint8_t buf[2] = {0, 0};
    i2c_transfer7(i2c, VL53L0X_ADDR, &reg, 1, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}


static const uint8_t init_seq[][2] = {
    {0x88, 0x00},
    {0x80, 0x01},
    {0xFF, 0x01},
    {0x00, 0x00},
    {0x91, 0x3C},   
    {0x00, 0x01},
    {0xFF, 0x00},
    {0x80, 0x00},
    {0xFF, 0x01},
    {0x4F, 0x00},
    {0x4E, 0x2C},
    {0xFF, 0x00},
    {0x44, 0x00},
    {0x45, 0x20},
    {0x47, 0x08},
    {0x48, 0x28},
    {0x67, 0x00},
    {0x70, 0x04},
    {0x39, 0x83},
    {0x0D, 0x01},
    {0xFF, 0x01},
    {0x00, 0x00},
    {0xFF, 0x00},
    {0x44, 0x00},
    {0x01, 0xFF},   
    {0x0D, 0x00},
    {0x80, 0x01},
    {0x01, 0xFE},
    {0xFF, 0x01},
    {0x8E, 0x01},
    {0x00, 0x01},
    {0xFF, 0x00},
    {0x80, 0x00},
};

bool vl53l0x_init(uint32_t i2c)
{
    
    uint8_t id = reg_read(i2c, 0xC0);
    if (id != VL53L0X_MODEL_ID) return false;

   
    for (uint32_t i = 0; i < sizeof(init_seq)/sizeof(init_seq[0]); i++) {
        reg_write(i2c, init_seq[i][0], init_seq[i][1]);
    }

    reg_write(i2c, 0x70, 0x04);
    reg_write(i2c, 0x01, 0xE8);
    reg_write(i2c, 0x80, 0x01);
    reg_write(i2c, 0xFF, 0x01);
    reg_write(i2c, 0x00, 0x00);
    reg_write(i2c, 0x91, 0x3C);
    reg_write(i2c, 0x00, 0x01);
    reg_write(i2c, 0xFF, 0x00);
    reg_write(i2c, 0x80, 0x00);
    reg_write(i2c, 0x00, 0x02);

    return true;
}

uint16_t vl53l0x_read_range_mm(uint32_t i2c)
{
    uint32_t timeout = 500;   
    while (!(reg_read(i2c, 0x13) & 0x07)) {
        if (--timeout == 0) return 0xFFFF;
        for (volatile int i = 0; i < 48000; i++) __asm__("nop");
    }
    uint16_t range = reg_read16(i2c, 0x1E);

    reg_write(i2c, 0x0B, 0x01);
    if (range >= 8190) return 0xFFFF;

    return range;
}

void vl53l0x_start_continuous(uint32_t i2c, uint32_t period_ms)
{
    reg_write(i2c, 0x80, 0x01);
    reg_write(i2c, 0xFF, 0x01);
    reg_write(i2c, 0x00, 0x00);
    reg_write(i2c, 0x91, 0x3C);
    reg_write(i2c, 0x00, 0x01);
    reg_write(i2c, 0xFF, 0x00);
    reg_write(i2c, 0x80, 0x00);

    if (period_ms) {
        uint32_t osc_calib = reg_read16(i2c, 0xF8);
        uint32_t inter_meas = (uint32_t)((period_ms / 10.0f) * osc_calib * 65536.0f);
        uint8_t buf[5] = {
            0x04,
            (inter_meas >> 24) & 0xFF,
            (inter_meas >> 16) & 0xFF,
            (inter_meas >>  8) & 0xFF,
            (inter_meas      ) & 0xFF,
        };
        i2c_transfer7(i2c, VL53L0X_ADDR, buf, 5, NULL, 0);
        reg_write(i2c, 0x00, 0x04);  
    } else {
        reg_write(i2c, 0x00, 0x02);  
    }
}

void vl53l0x_stop_continuous(uint32_t i2c)
{
    reg_write(i2c, 0x00, 0x01);
    reg_write(i2c, 0xFF, 0x01);
    reg_write(i2c, 0x00, 0x00);
    reg_write(i2c, 0x91, 0x00);
    reg_write(i2c, 0x00, 0x01);
    reg_write(i2c, 0xFF, 0x00);
}
