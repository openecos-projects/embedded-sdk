#ifndef ST7789_TYPE_H
#define ST7789_TYPE_H

#include <stdint.h>

typedef struct st7789_t{
    uint32_t dc_pin;
    uint32_t rst_pin;
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t rotation;
    uint8_t horizontal_offset;
    uint8_t vertical_offset;
} st7789_device_t;

#endif