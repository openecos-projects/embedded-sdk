#include "ecos/hal/timer_interrupt.h"
#include "ecos/hal/interrupt.h"
#include "cl1_2512_soc.h"
#include "timer_internal.h"

#include <stddef.h>

static hal_timer_interrupt_handler_t timer_callback;
static void *timer_callback_arg;

static void timer_interrupt_dispatch(void *arg)
{
    hal_timer_interrupt_handler_t callback = timer_callback;
    (void)arg;
    (void)hal_timer_clear_interrupt(0u);
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    if (callback != NULL)
        callback(timer_callback_arg);
}

ecos_err_t hal_timer_clear_interrupt(hal_timer_id_t timer)
{
    uint32_t eoi;

    if (timer != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    eoi = REG_TIMER_0_EOI;
    (void)eoi;
    return ECOS_OK;
}

ecos_err_t hal_timer_register_callback(hal_timer_id_t timer,
                                       hal_timer_interrupt_handler_t callback,
                                       void *arg,
                                       uint32_t priority)
{
    if (timer != 0u || callback == NULL || priority == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_callback != NULL || cl1_2512_timer_interrupt_active != 0u)
        return ECOS_ERR_BUSY;
    if (REG_TIMER_0_LOAD_COUNT == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    timer_callback_arg = arg;
    timer_callback = callback;
    if (hal_interrupt_alloc(3u, priority, timer_interrupt_dispatch, NULL) !=
        ECOS_OK) {
        timer_callback = NULL;
        timer_callback_arg = NULL;
        return ECOS_ERR_IO;
    }
    cl1_2512_timer_interrupt_active = 1u;
    REG_TIMER_0_CONTROL &= ~CL1_2512_TIMER_CONTROL_MASK;
    if (hal_interrupt_enable(3u) != ECOS_OK) {
        REG_TIMER_0_CONTROL |= CL1_2512_TIMER_CONTROL_MASK;
        (void)hal_interrupt_free(3u);
        cl1_2512_timer_interrupt_active = 0u;
        timer_callback = NULL;
        timer_callback_arg = NULL;
        return ECOS_ERR_IO;
    }
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}

ecos_err_t hal_timer_unregister_callback(hal_timer_id_t timer)
{
    if (timer != 0u || timer_callback == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_TIMER_0_CONTROL |= CL1_2512_TIMER_CONTROL_MASK;
    (void)hal_interrupt_disable(3u);
    (void)hal_timer_clear_interrupt(0u);
    if (hal_interrupt_free(3u) != ECOS_OK)
        return ECOS_ERR_IO;
    cl1_2512_timer_interrupt_active = 0u;
    timer_callback = NULL;
    timer_callback_arg = NULL;
    return ECOS_OK;
}
