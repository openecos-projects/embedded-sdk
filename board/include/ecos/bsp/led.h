#ifndef ECOS_BSP_LED_H
#define ECOS_BSP_LED_H

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

enum {
    BSP_LED_OK = 0,
    BSP_LED_ERROR_INVALID_ARGUMENT = -1,
    BSP_LED_ERROR_UNSUPPORTED = -2,
    BSP_LED_ERROR_IO = -3,
    BSP_LED_ERROR_NOT_INITIALIZED = -4
};

/* Configure all board LEDs as GPIO outputs and leave them off. */
int bsp_led_init(void);

int bsp_led_set_state(bsp_led_t led, bsp_led_state_t state);

#ifdef __cplusplus
}
#endif

#endif
