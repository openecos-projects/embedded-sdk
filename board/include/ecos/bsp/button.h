#ifndef ECOS_BSP_BUTTON_H
#define ECOS_BSP_BUTTON_H

#include "ecos/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_BUTTON_0 = 0,
    BSP_BUTTON_1,
    BSP_BUTTON_COUNT
} bsp_button_t;

typedef enum {
    BSP_BUTTON_RELEASED = 0,
    BSP_BUTTON_PRESSED
} bsp_button_state_t;

/* Configure all board buttons as GPIO inputs. */
ecos_err_t bsp_button_init(void);

/* Returns BSP_BUTTON_RELEASED/PRESSED or a negative error code. */
int bsp_button_get_state(bsp_button_t button);

#ifdef __cplusplus
}
#endif

#endif
