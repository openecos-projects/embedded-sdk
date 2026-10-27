#include "ecos/driver/usb.h"
#include "ecos/hal/usb.h"

ecos_err_t ecos_usb_init(ecos_usb_id_t id, const ecos_usb_config_t *config)
{
    return hal_usb_init((hal_usb_id_t)id, (const hal_usb_config_t *)config);
}
ecos_err_t ecos_usb_deinit(ecos_usb_id_t id)
{
    return hal_usb_deinit((hal_usb_id_t)id);
}
ecos_err_t ecos_usb_get_status(ecos_usb_id_t id, uint32_t *status)
{
    return hal_usb_get_status((hal_usb_id_t)id, status);
}
ecos_err_t ecos_usb_clear_interrupts(ecos_usb_id_t id, uint8_t flags)
{
    return hal_usb_clear_interrupts((hal_usb_id_t)id, flags);
}
