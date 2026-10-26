#include "ecos/hal/timer_interrupt.h"
#include "ecos/hal/interrupt.h"
#include "ch32v103_timer_internal.h"

#include <stddef.h>

static hal_timer_interrupt_handler_t timer_callback;
static void *timer_arg;

static void timer_dispatch(void *arg)
{
    (void)arg;
    if ((ch32v103_read16(CH32V103_TIM2_INTFR) & CH32V103_TIM_UIF) == 0u ||
        (ch32v103_read16(CH32V103_TIM2_DMAINTENR) & CH32V103_TIM_UIE) == 0u)
        return;
    (void)hal_timer_clear_interrupt(0u);
    if (timer_callback != NULL)
        timer_callback(timer_arg);
}

ecos_err_t hal_timer_register_callback(hal_timer_id_t timer,
                                       hal_timer_interrupt_handler_t callback,
                                       void *arg, uint32_t priority)
{
    uint32_t state;
    ecos_err_t result;
    if (timer != 0u || callback == NULL || priority == 0u ||
        priority > CH32V103_IRQ_PRIORITY_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (ch32v103_timer_initialized == 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    if (ch32v103_timer_irq_active != 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    if ((ch32v103_read16(CH32V103_TIM2_CTLR1) & CH32V103_TIM_CEN) != 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY; /* register callbacks while the timer is stopped */
    }
    result = hal_interrupt_alloc(CH32V103_IRQ_TIM2, priority, timer_dispatch, NULL);
    if (result == ECOS_OK) {
        timer_callback = callback;
        timer_arg = arg;
        ch32v103_timer_irq_active = 1u;
        (void)hal_timer_clear_interrupt(timer);
        ch32v103_write16(CH32V103_TIM2_DMAINTENR,
                         ch32v103_read16(CH32V103_TIM2_DMAINTENR) | CH32V103_TIM_UIE);
        result = hal_interrupt_enable(CH32V103_IRQ_TIM2);
        if (result != ECOS_OK) {
            ch32v103_write16(CH32V103_TIM2_DMAINTENR, 0u);
            ch32v103_timer_irq_active = 0u;
            timer_callback = NULL;
            timer_arg = NULL;
            (void)hal_interrupt_free(CH32V103_IRQ_TIM2);
        }
    }
    ch32v103_irq_restore(state);
    return result;
}

ecos_err_t hal_timer_unregister_callback(hal_timer_id_t timer)
{
    uint32_t state;
    ecos_err_t result;
    if (timer != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (ch32v103_timer_irq_active == 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    ch32v103_write16(CH32V103_TIM2_DMAINTENR,
                     ch32v103_read16(CH32V103_TIM2_DMAINTENR) & (uint16_t)~CH32V103_TIM_UIE);
    (void)hal_interrupt_disable(CH32V103_IRQ_TIM2);
    (void)hal_timer_clear_interrupt(timer);
    result = hal_interrupt_free(CH32V103_IRQ_TIM2);
    if (result == ECOS_OK) {
        ch32v103_timer_irq_active = 0u;
        timer_callback = NULL;
        timer_arg = NULL;
    }
    ch32v103_irq_restore(state);
    return result;
}
