#ifndef ECOS_DRIVER_USB_H
#define ECOS_DRIVER_USB_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_usb_id_t;
typedef struct {
    uint8_t low_speed;
    uint8_t internal_pullup;
} ecos_usb_config_t;
#define ECOS_USB_DEFAULT ((ecos_usb_id_t)0u)
#define ECOS_USB_CONFIG_DEFAULT { 0u, 1u }

ecos_err_t ecos_usb_init(ecos_usb_id_t id, const ecos_usb_config_t *config);
ecos_err_t ecos_usb_deinit(ecos_usb_id_t id);
ecos_err_t ecos_usb_get_status(ecos_usb_id_t id, uint32_t *status);
ecos_err_t ecos_usb_clear_interrupts(ecos_usb_id_t id, uint8_t flags);

#ifdef __cplusplus
}
#endif

#endif
