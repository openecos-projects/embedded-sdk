#ifndef ECOS_BSP_LED_H
#define ECOS_BSP_LED_H

#include "ecos/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_LED_0 = 0,
    BSP_LED_1,
    BSP_LED_COUNT
} bsp_led_t;

typedef enum {
    BSP_LED_OFF = 0,
    BSP_LED_ON
} bsp_led_state_t;

/* Configure all board LEDs as GPIO outputs and leave them off. */
ecos_err_t bsp_led_init(void);

ecos_err_t bsp_led_set_state(bsp_led_t led, bsp_led_state_t state);

#ifdef __cplusplus
}
#endif

#endif
