/* CH32V103 TIM2 polling/interrupt time base, fixed HSI/APB1 at 8 MHz.
 * 16-bit CNT and PSC use a power-of-two microsecond quantum for long periods.
 * Periods round up by less than one quantum; get_count returns microseconds.
 */
#include "ecos/hal/timer.h"
#include "ecos/hal/timer_interrupt.h"
#include "ch32v103_timer_internal.h"

#include <stddef.h>

#define TIM_URS (1u << 2)
#define TIM_ARPE (1u << 7)
#define TIMER_MAX_QUANTUM_US 8192u

uint8_t ch32v103_timer_initialized;
uint8_t ch32v103_timer_irq_active;
static uint32_t quantum_us;

static int valid_timer(hal_timer_id_t timer)
{
    return timer == 0u;
}

static void clear_update_flag(void)
{
    /* INTFR flags clear on writing zero (W0C); ones preserve other flags. */
    ch32v103_write16(CH32V103_TIM2_INTFR, (uint16_t)~CH32V103_TIM_UIF);
    ch32v103_io_fence();
}

int hal_timer_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_timer_init(hal_timer_id_t timer, const hal_timer_config_t *config)
{
    uint32_t quantum = 1u;
    uint32_t count;
    uint32_t state;
    if (!valid_timer(timer) || config == NULL || config->period_ticks == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->period_ticks > CH32V103_TIMER_MAX_PERIOD_US)
        return ECOS_ERR_UNSUPPORTED;
    state = ch32v103_irq_lock();
    if (ch32v103_timer_irq_active != 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    while ((config->period_ticks + quantum - 1u) / quantum > 65536u &&
           quantum < TIMER_MAX_QUANTUM_US)
        quantum <<= 1;
    count = (config->period_ticks + quantum - 1u) / quantum;
    ch32v103_write32(CH32V103_RCC_APB1PCENR,
                     ch32v103_read32(CH32V103_RCC_APB1PCENR) | 1u);
    ch32v103_write16(CH32V103_TIM2_CTLR1, TIM_URS | TIM_ARPE);
    ch32v103_write16(CH32V103_TIM2_BASE + 0x04u, 0u); /* CTLR2 */
    ch32v103_write16(CH32V103_TIM2_BASE + 0x08u, 0u); /* SMCFGR: internal clock */
    ch32v103_write16(CH32V103_TIM2_DMAINTENR, 0u);
    ch32v103_write16(CH32V103_TIM2_PSC, (uint16_t)(8u * quantum - 1u));
    ch32v103_write16(CH32V103_TIM2_ATRLR, (uint16_t)(count - 1u));
    ch32v103_write16(CH32V103_TIM2_CNT, 0u);
    ch32v103_write16(CH32V103_TIM2_SWEVGR, 1u);
    clear_update_flag();
    quantum_us = quantum;
    ch32v103_timer_initialized = 1u;
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_timer_deinit(hal_timer_id_t timer)
{
    uint32_t state;
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (ch32v103_timer_irq_active != 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    if (ch32v103_timer_initialized == 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    ch32v103_write16(CH32V103_TIM2_CTLR1, 0u);
    ch32v103_write16(CH32V103_TIM2_DMAINTENR, 0u);
    clear_update_flag();
    ch32v103_write16(CH32V103_TIM2_CNT, 0u);
    ch32v103_timer_initialized = 0u;
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_timer_start(hal_timer_id_t timer)
{
    uint32_t state;
    uint16_t control;
    uint16_t interrupts;
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (ch32v103_timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    state = ch32v103_irq_lock();
    control = ch32v103_read16(CH32V103_TIM2_CTLR1);
    interrupts = ch32v103_read16(CH32V103_TIM2_DMAINTENR);
    ch32v103_write16(CH32V103_TIM2_CTLR1, control & (uint16_t)~CH32V103_TIM_CEN);
    ch32v103_write16(CH32V103_TIM2_DMAINTENR, 0u);
    /* UG resets the prescaler as well as CNT, making start restart a full period. */
    ch32v103_write16(CH32V103_TIM2_CNT, 0u);
    ch32v103_write16(CH32V103_TIM2_SWEVGR, 1u);
    clear_update_flag();
    ch32v103_write16(CH32V103_TIM2_DMAINTENR, interrupts);
    ch32v103_write16(CH32V103_TIM2_CTLR1, control | CH32V103_TIM_CEN);
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_timer_stop(hal_timer_id_t timer)
{
    uint32_t state;
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (ch32v103_timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    state = ch32v103_irq_lock();
    ch32v103_write16(CH32V103_TIM2_CTLR1,
                     ch32v103_read16(CH32V103_TIM2_CTLR1) & (uint16_t)~CH32V103_TIM_CEN);
    clear_update_flag();
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_timer_get_count(hal_timer_id_t timer, uint32_t *count)
{
    if (!valid_timer(timer) || count == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (ch32v103_timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    *count = (uint32_t)ch32v103_read16(CH32V103_TIM2_CNT) * quantum_us;
    return ECOS_OK;
}

int hal_timer_is_expired(hal_timer_id_t timer)
{
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (ch32v103_timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    return (ch32v103_read16(CH32V103_TIM2_INTFR) & CH32V103_TIM_UIF) != 0u;
}

ecos_err_t hal_timer_delay_us(hal_timer_id_t timer, uint32_t duration_us)
{
    if (!valid_timer(timer) || duration_us == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (ch32v103_timer_irq_active != 0u)
        return ECOS_ERR_BUSY;
    while (duration_us != 0u) {
        uint32_t chunk = duration_us > CH32V103_TIMER_MAX_PERIOD_US ?
                         CH32V103_TIMER_MAX_PERIOD_US : duration_us;
        hal_timer_config_t config = { chunk };
        int expired;
        ecos_err_t result = hal_timer_init(timer, &config);
        if (result != ECOS_OK)
            return result;
        result = hal_timer_start(timer);
        if (result != ECOS_OK) {
            (void)hal_timer_deinit(timer);
            return result;
        }
        do {
            expired = hal_timer_is_expired(timer);
        } while (expired == 0);
        (void)hal_timer_deinit(timer);
        if (expired < 0)
            return (ecos_err_t)expired;
        duration_us -= chunk;
    }
    return ECOS_OK;
}

uint8_t hal_delay_us(uint8_t timer_id, uint32_t value)
{
    return hal_timer_delay_us(timer_id, value) == ECOS_OK ? 0u : 1u;
}

uint8_t hal_delay_ms(uint8_t timer_id, uint32_t value)
{
    while (value != 0u) {
        if (hal_timer_delay_us(timer_id, 1000u) != ECOS_OK)
            return 1u;
        --value;
    }
    return 0u;
}

uint8_t hal_delay_s(uint8_t timer_id, uint32_t value)
{
    while (value != 0u) {
        if (hal_delay_ms(timer_id, 1000u) != 0u)
            return 1u;
        --value;
    }
    return 0u;
}

uint8_t hal_sys_tick_init(uint8_t timer_id)
{
    const hal_timer_config_t config = { 65536u };
    if (hal_timer_init(timer_id, &config) != ECOS_OK)
        return 1u;
    return hal_timer_start(timer_id) == ECOS_OK ? 0u : 1u;
}

uint32_t hal_get_sys_tick(uint8_t timer_id)
{
    uint32_t count = 0u;
    (void)hal_timer_get_count(timer_id, &count);
    return count;
}

ecos_err_t hal_timer_clear_interrupt(hal_timer_id_t timer)
{
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (ch32v103_timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    clear_update_flag();
    return ECOS_OK;
}
