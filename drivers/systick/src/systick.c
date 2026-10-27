#include "ecos/driver/systick.h"
#include "ecos/hal/systick.h"

ecos_err_t ecos_systick_init(const ecos_systick_config_t *config,
                             ecos_systick_callback_t callback, void *arg,
                             uint32_t priority)
{
    return hal_systick_init((const hal_systick_config_t *)config,
                            (hal_systick_callback_t)callback, arg, priority);
}
ecos_err_t ecos_systick_start(void) { return hal_systick_start(); }
ecos_err_t ecos_systick_stop(void) { return hal_systick_stop(); }
ecos_err_t ecos_systick_get_counter(uint64_t *counter)
{
    return hal_systick_get_counter(counter);
}
