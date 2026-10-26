/* Minimal CH32V103 TIM2 polling backend.
 * TIM2 runs from the 8 MHz HSI with a divide-by-eight prescaler, so one
 * counter tick is one microsecond while the APB1 clock remains undivided.
 */
#include "ecos/hal/timer.h"
#include "ecos/hal/timer_interrupt.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define RCC_APB1PCENR REG32(0x4002101Cu)
#define TIM2_BASE     0x40000000u
#define TIM2_CTLR1    REG16(TIM2_BASE + 0x00u)
#define TIM2_INTFR    REG16(TIM2_BASE + 0x10u)
#define TIM2_SWEVGR   REG16(TIM2_BASE + 0x14u)
#define TIM2_CNT      REG16(TIM2_BASE + 0x24u)
#define TIM2_PSC      REG16(TIM2_BASE + 0x28u)
#define TIM2_ATRLR    REG16(TIM2_BASE + 0x2Cu)

#define TIMER_CEN     0x0001u
#define TIMER_UIF     0x0001u
#define TIMER_PSC     7u
#define TIMER_MAX_US  65536u

static uint8_t timer_initialized;

static int valid_timer(hal_timer_id_t timer)
{
    return timer == 0u;
}

static void enable_timer_clock(void)
{
    /* RCC_APB1PCENR TIM2EN is bit 0. */
    RCC_APB1PCENR |= 1u;
}

static void clear_update_flag(void)
{
    /* CH32 TIMx INTFR uses the WCH write-one-to-clear convention: the
     * vendor library writes the complemented flag mask. */
    TIM2_INTFR = (uint16_t)~TIMER_UIF;
}

int hal_timer_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_timer_init(hal_timer_id_t timer,
                          const hal_timer_config_t *config)
{
    if (!valid_timer(timer) || config == NULL || config->period_ticks == 0u ||
        config->period_ticks > TIMER_MAX_US)
        return ECOS_ERR_INVALID_ARGUMENT;

    enable_timer_clock();
    TIM2_CTLR1 &= (uint16_t)~TIMER_CEN;
    TIM2_PSC = TIMER_PSC;
    TIM2_ATRLR = (uint16_t)(config->period_ticks - 1u);
    TIM2_CNT = 0u;
    TIM2_SWEVGR = 1u;
    clear_update_flag();
    timer_initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_timer_deinit(hal_timer_id_t timer)
{
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    TIM2_CTLR1 &= (uint16_t)~TIMER_CEN;
    clear_update_flag();
    TIM2_CNT = 0u;
    TIM2_PSC = 0u;
    TIM2_ATRLR = 0u;
    timer_initialized = 0u;
    return ECOS_OK;
}

ecos_err_t hal_timer_start(hal_timer_id_t timer)
{
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    TIM2_CTLR1 &= (uint16_t)~TIMER_CEN;
    TIM2_CNT = 0u;
    clear_update_flag();
    TIM2_CTLR1 |= TIMER_CEN;
    return ECOS_OK;
}

ecos_err_t hal_timer_stop(hal_timer_id_t timer)
{
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    TIM2_CTLR1 &= (uint16_t)~TIMER_CEN;
    clear_update_flag();
    return ECOS_OK;
}

ecos_err_t hal_timer_get_count(hal_timer_id_t timer, uint32_t *count)
{
    if (!valid_timer(timer) || count == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    *count = TIM2_CNT;
    return ECOS_OK;
}

int hal_timer_is_expired(hal_timer_id_t timer)
{
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    return (TIM2_INTFR & TIMER_UIF) != 0u ? 1 : 0;
}

ecos_err_t hal_timer_delay_us(hal_timer_id_t timer, uint32_t duration_us)
{
    if (!valid_timer(timer) || duration_us == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    while (duration_us != 0u) {
        uint32_t chunk = duration_us > TIMER_MAX_US ? TIMER_MAX_US : duration_us;
        hal_timer_config_t config = { chunk };
        int expired;
        ecos_err_t result = hal_timer_init(timer, &config);

        if (result != ECOS_OK)
            return result;
        result = hal_timer_start(timer);
        if (result != ECOS_OK)
            return result;
        do {
            expired = hal_timer_is_expired(timer);
        } while (expired == 0);
        if (expired < 0)
            return (ecos_err_t)expired;
        result = hal_timer_deinit(timer);
        if (result != ECOS_OK)
            return result;
        duration_us -= chunk;
    }
    return ECOS_OK;
}

/* Legacy delay entry points used by existing device code. */
uint8_t hal_delay_us(uint8_t timer_id, uint32_t value)
{
    return hal_timer_delay_us(timer_id, value) == ECOS_OK ? 0u : 1u;
}

uint8_t hal_delay_ms(uint8_t timer_id, uint32_t value)
{
    while (value != 0u) {
        if (hal_timer_delay_us(timer_id, 1000u) != 0u)
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
    const hal_timer_config_t config = { TIMER_MAX_US };

    if (hal_timer_init(timer_id, &config) != ECOS_OK)
        return 1u;
    return hal_timer_start(timer_id) == ECOS_OK ? 0u : 1u;
}

uint32_t hal_get_sys_tick(uint8_t timer_id)
{
    uint32_t count = 0u;

    if (hal_timer_get_count(timer_id, &count) != ECOS_OK)
        return 0u;
    return count;
}

ecos_err_t hal_timer_clear_interrupt(hal_timer_id_t timer)
{
    if (!valid_timer(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    clear_update_flag();
    return ECOS_OK;
}

ecos_err_t hal_timer_register_callback(
    hal_timer_id_t timer,
    hal_timer_interrupt_handler_t callback,
    void *arg,
    uint32_t priority
)
{
    (void)timer;
    (void)callback;
    (void)arg;
    (void)priority;
    return ECOS_ERR_UNSUPPORTED;
}

ecos_err_t hal_timer_unregister_callback(hal_timer_id_t timer)
{
    (void)timer;
    return ECOS_ERR_UNSUPPORTED;
}
