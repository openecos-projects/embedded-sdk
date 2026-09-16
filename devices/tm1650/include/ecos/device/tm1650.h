#ifndef ECOS_DEVICE_TM1650_H
#define ECOS_DEVICE_TM1650_H

#include "ecos/driver/gpio.h"
#include "ecos/error.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TM1650 drives 4 digits; segments are bit0=A .. bit6=G, bit7=DP. */
#define ECOS_TM1650_POSITION_COUNT 4u
#define ECOS_TM1650_SEGMENT_DOT 0x80u
#define ECOS_TM1650_BRIGHTNESS_OFF 0u
#define ECOS_TM1650_BRIGHTNESS_MAX 8u
#define ECOS_TM1650_BRIGHTNESS_DEFAULT 3u

typedef struct {
    ecos_gpio_port_t dat_port;
    uint8_t dat_pin;
    ecos_gpio_port_t clk_port;
    uint8_t clk_pin;
} ecos_tm1650_config_t;

typedef struct {
    ecos_tm1650_config_t config;
    uint8_t brightness;
    uint8_t initialized;
} ecos_tm1650_t;

/* StarrySky L4C1 on-board wiring: SEG_DAT = GPIO1[9], SEG_CLK = GPIO1[10]. */
#define ECOS_TM1650_CONFIG_DEFAULT \
    { ECOS_GPIO_PORT_1, 9u, ECOS_GPIO_PORT_1, 10u }

/* Configures the GPIO pins, clears all digits and turns the display on
 * at ECOS_TM1650_BRIGHTNESS_DEFAULT. */
ecos_err_t ecos_tm1650_init(ecos_tm1650_t *display,
                            const ecos_tm1650_config_t *config);
ecos_err_t ecos_tm1650_deinit(ecos_tm1650_t *display);

/* Level 0 turns the display off, 1..8 select the duty ratio. */
ecos_err_t ecos_tm1650_set_brightness(ecos_tm1650_t *display, uint8_t level);

ecos_err_t ecos_tm1650_clear(ecos_tm1650_t *display);

/* Raw segment pattern for one position (0 = DIG1 .. 3 = DIG4). */
ecos_err_t ecos_tm1650_set_segments(ecos_tm1650_t *display,
                                    uint8_t position,
                                    uint8_t segments);

/* Shows a hexadecimal digit (0-15) at one position. */
ecos_err_t ecos_tm1650_show_digit(ecos_tm1650_t *display,
                                  uint8_t position,
                                  uint8_t value,
                                  bool dot);

/* Shows an unsigned decimal number (0-9999) right-aligned across the
 * 4 digits; leading positions are blank unless leading_zero is set. */
ecos_err_t ecos_tm1650_show_number(ecos_tm1650_t *display,
                                   uint16_t value,
                                   bool leading_zero);

#ifdef __cplusplus
}
#endif

#endif
