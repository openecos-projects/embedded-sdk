#ifndef ECOS_DEVICE_ST7735_H
#define ECOS_DEVICE_ST7735_H

#include "ecos/driver/gpio.h"
#include "ecos/driver/qspi.h"
#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ecos_qspi_id_t qspi;
    ecos_qspi_cs_t chip_select;
    ecos_gpio_port_t dc_port;
    uint8_t dc_pin;
    /* RST and BLK are configured high once during initialization. */
    ecos_gpio_port_t reset_port;
    uint8_t reset_pin;
    ecos_gpio_port_t backlight_port;
    uint8_t backlight_pin;
    uint16_t width;
    uint16_t height;
    uint8_t rotation;
    uint8_t horizontal_offset;
    uint8_t vertical_offset;
    uint32_t qspi_clock_divider;
} ecos_st7735_config_t;

typedef struct {
    ecos_st7735_config_t config;
    uint8_t initialized;
} ecos_st7735_t;

#define ECOS_ST7735_CONFIG_DEFAULT \
    { ECOS_QSPI_DEFAULT, ECOS_QSPI_CS_0, ECOS_GPIO_PORT_0, 29u, \
      ECOS_GPIO_PORT_0, 30u, ECOS_GPIO_PORT_0, 31u, \
      128u, 128u, 0u, 0u, 0u, 3u }

ecos_err_t ecos_st7735_init(ecos_st7735_t *display,
                            const ecos_st7735_config_t *config);
ecos_err_t ecos_st7735_deinit(ecos_st7735_t *display);

ecos_err_t ecos_st7735_set_window(ecos_st7735_t *display,
                                  uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  uint16_t height);

/* Color is two packed RGB565 pixels, high half sent first. */
ecos_err_t ecos_st7735_fill(ecos_st7735_t *display,
                            uint16_t x,
                            uint16_t y,
                            uint16_t width,
                            uint16_t height,
                            uint32_t color);

#ifdef __cplusplus
}
#endif

#endif
