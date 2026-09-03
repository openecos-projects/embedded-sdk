#include "ecos/driver/timer.h"

#include "ecos/hal/timer.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_MILLISECONDS_PER_DELAY      4294967u
#define MICROSECONDS_PER_SECOND         1000000u

static int timer_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

int ecos_timer_get_instance_count(void)
{
    return timer_map_hal_result(hal_timer_get_instance_count());
}

ecos_err_t ecos_timer_init(ecos_timer_id_t timer,
                           const ecos_timer_config_t *config)
{
    hal_timer_config_t hal_config;

    if (config == NULL || config->period_us == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.period_ticks = config->period_us;
    return timer_map_hal_result(
        hal_timer_init((hal_timer_id_t)timer, &hal_config)
    );
}

ecos_err_t ecos_timer_deinit(ecos_timer_id_t timer)
{
    return timer_map_hal_result(hal_timer_deinit((hal_timer_id_t)timer));
}

ecos_err_t ecos_timer_start(ecos_timer_id_t timer)
{
    return timer_map_hal_result(hal_timer_start((hal_timer_id_t)timer));
}

ecos_err_t ecos_timer_stop(ecos_timer_id_t timer)
{
    return timer_map_hal_result(hal_timer_stop((hal_timer_id_t)timer));
}

ecos_err_t ecos_timer_get_count(ecos_timer_id_t timer, uint32_t *count)
{
    if (count == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    return timer_map_hal_result(
        hal_timer_get_count((hal_timer_id_t)timer, count)
    );
}

int ecos_timer_is_expired(ecos_timer_id_t timer)
{
    return timer_map_hal_result(
        hal_timer_is_expired((hal_timer_id_t)timer)
    );
}

ecos_err_t ecos_timer_delay_us(ecos_timer_id_t timer, uint32_t duration_us)
{
    if (duration_us == 0u)
        return ECOS_OK;

    return timer_map_hal_result(
        hal_timer_delay_us((hal_timer_id_t)timer, duration_us)
    );
}

ecos_err_t ecos_timer_delay_ms(ecos_timer_id_t timer, uint32_t duration_ms)
{
    while (duration_ms != 0u) {
        uint32_t current_ms = duration_ms;
        uint32_t duration_us;
        int result;

        if (current_ms > MAX_MILLISECONDS_PER_DELAY)
            current_ms = MAX_MILLISECONDS_PER_DELAY;
        /* 1000 = 1024 - 16 - 8，避免引入 RV32E 软件乘法函数。 */
        duration_us = (current_ms << 10) -
                      (current_ms << 4) -
                      (current_ms << 3);
        result = ecos_timer_delay_us(timer, duration_us);
        if (result != ECOS_OK)
            return result;
        duration_ms -= current_ms;
    }
    return ECOS_OK;
}

ecos_err_t ecos_timer_delay_s(ecos_timer_id_t timer, uint32_t duration_s)
{
    while (duration_s != 0u) {
        int result = ecos_timer_delay_us(timer, MICROSECONDS_PER_SECOND);

        if (result != ECOS_OK)
            return result;
        --duration_s;
    }
    return ECOS_OK;
}
