#include "ecos/hal/timer.h"
#include "cl1_2512_soc.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define CL1_2512_TIMER_COUNT        1u
#define TIMER_TICKS_PER_MICROSECOND \
    (CL1_2512_TIMER_CLOCK_HZ / 1000000u)
#define TIMER_MAX_PERIOD_US \
    (UINT32_MAX / TIMER_TICKS_PER_MICROSECOND)
#define TIMER_CONTROL_ENABLE        (1u << 0)
#define TIMER_CONTROL_USER_DEFINED  (1u << 1)

static uint8_t timer_initialized[CL1_2512_TIMER_COUNT];

static int timer_id_is_valid(hal_timer_id_t timer)
{
    return timer < CL1_2512_TIMER_COUNT;
}

static void timer_stop_and_clear(void)
{
    uint32_t eoi;

    REG_TIMER_0_CONTROL = 0u;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    eoi = REG_TIMER_0_EOI;
    (void)eoi;
}

int hal_timer_get_instance_count(void)
{
    return (int)CL1_2512_TIMER_COUNT;
}

ecos_err_t hal_timer_init(hal_timer_id_t timer,
                          const hal_timer_config_t *config)
{
    if (!timer_id_is_valid(timer) || config == NULL ||
        config->period_ticks == 0u || TIMER_TICKS_PER_MICROSECOND == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->period_ticks > UINT32_MAX / TIMER_TICKS_PER_MICROSECOND)
        return ECOS_ERR_UNSUPPORTED;

    timer_stop_and_clear();
    REG_TIMER_0_LOAD_COUNT =
        config->period_ticks * TIMER_TICKS_PER_MICROSECOND;
    REG_TIMER_0_CONTROL = TIMER_CONTROL_USER_DEFINED;
    timer_initialized[timer] = 1u;
    return ECOS_OK;
}

ecos_err_t hal_timer_deinit(hal_timer_id_t timer)
{
    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;

    timer_stop_and_clear();
    REG_TIMER_0_LOAD_COUNT = 0u;
    timer_initialized[timer] = 0u;
    return ECOS_OK;
}

ecos_err_t hal_timer_start(hal_timer_id_t timer)
{
    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    timer_stop_and_clear();
    REG_TIMER_0_CONTROL = TIMER_CONTROL_USER_DEFINED |
                          TIMER_CONTROL_ENABLE;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}

ecos_err_t hal_timer_stop(hal_timer_id_t timer)
{
    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    REG_TIMER_0_CONTROL &= ~TIMER_CONTROL_ENABLE;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}

ecos_err_t hal_timer_get_count(hal_timer_id_t timer, uint32_t *count)
{
    if (!timer_id_is_valid(timer) || count == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    *count = REG_TIMER_0_CURRENT_VALUE / TIMER_TICKS_PER_MICROSECOND;
    return ECOS_OK;
}

int hal_timer_is_expired(hal_timer_id_t timer)
{
    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    return REG_TIMER_0_INT_STATUS != 0u ? 1 : 0;
}

ecos_err_t hal_timer_delay_us(hal_timer_id_t timer, uint32_t duration_us)
{
    hal_timer_config_t config;

    if (!timer_id_is_valid(timer) || duration_us == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    while (duration_us != 0u) {
        int result;

        config.period_ticks = duration_us;
        if (config.period_ticks > TIMER_MAX_PERIOD_US)
            config.period_ticks = TIMER_MAX_PERIOD_US;

        result = hal_timer_init(timer, &config);
        if (result != ECOS_OK)
            return result;
        result = hal_timer_start(timer);
        if (result != ECOS_OK) {
            (void)hal_timer_deinit(timer);
            return result;
        }
        do {
            result = hal_timer_is_expired(timer);
        } while (result == 0);
        if (result < 0) {
            (void)hal_timer_deinit(timer);
            return result;
        }
        result = hal_timer_deinit(timer);
        if (result != ECOS_OK)
            return result;
        duration_us -= config.period_ticks;
    }
    return ECOS_OK;
}
