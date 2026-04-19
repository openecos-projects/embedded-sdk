#ifndef ST7735_TYPE_H
#define ST7735_TYPE_H

#include <stdint.h>
#include "hal_qspi.h"

typedef struct st7735_t{
    uint32_t dc_gpio_port;
    uint32_t dc_gpio_pin;
    hal_qspi_port_t qspi_port;
    hal_qspi_cs_t qspi_cs;
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t rotation;
    uint8_t horizontal_offset;
    uint8_t vertical_offset;
} st7735_device_t;

#endif