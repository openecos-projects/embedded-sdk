#ifndef ECOS_BSP_DISPLAY_H
#define ECOS_BSP_DISPLAY_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

ecos_err_t bsp_display_init(void);
ecos_err_t bsp_display_deinit(void);
ecos_err_t bsp_display_fill_rect(uint16_t x, uint16_t y,
                                 uint16_t width, uint16_t height,
                                 uint16_t rgb565);
uint16_t bsp_display_width(void);
uint16_t bsp_display_height(void);

#ifdef __cplusplus
}
#endif

#endif
