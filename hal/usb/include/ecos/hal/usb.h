#ifndef ECOS_HAL_USB_H
#define ECOS_HAL_USB_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_usb_id_t;
#define HAL_USB_ID_0 0u

typedef struct {
    uint8_t low_speed;
    uint8_t internal_pullup;
} hal_usb_config_t;

ecos_err_t hal_usb_init(hal_usb_id_t id, const hal_usb_config_t *config);
ecos_err_t hal_usb_deinit(hal_usb_id_t id);
ecos_err_t hal_usb_get_status(hal_usb_id_t id, uint32_t *status);
ecos_err_t hal_usb_clear_interrupts(hal_usb_id_t id, uint8_t flags);

#ifdef __cplusplus
}
#endif

#endif
