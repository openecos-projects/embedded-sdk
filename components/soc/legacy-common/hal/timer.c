#include "ecos/hal/timer.h"
#include "legacy_soc.h"

#include <stddef.h>

#define TIMER_STOP 0u
#define TIMER_START 1u
#define TIMER_EXPIRED 1u

typedef struct {
    volatile uint32_t *control;
    volatile uint32_t *prescaler;
    volatile uint32_t *counter;
    volatile uint32_t *compare;
    volatile uint32_t *status;
    volatile uint32_t *legacy_value;
    volatile uint32_t *legacy_data;
} timer_registers_t;

static uint8_t initialized[4];
static uint32_t periods[4];

static int timer_valid(hal_timer_id_t timer)
{
    return timer < LEGACY_TIMER_COUNT;
}

static int timer_registers(hal_timer_id_t timer, timer_registers_t *r)
{
    if (r == NULL || !timer_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    r->legacy_value = NULL;
    r->legacy_data = NULL;
#if LEGACY_TIMER_STYLE_C
    if (timer == 0u) {
        r->control = &REG_TIM_0_CONFIG;
        r->counter = &REG_TIM_0_VALUE;
        r->legacy_data = &REG_TIM_0_DATA;
    } else {
        r->control = &REG_TIM_1_CONFIG;
        r->counter = &REG_TIM_1_VALUE;
        r->legacy_data = &REG_TIM_1_DATA;
    }
    r->prescaler = NULL;
    r->compare = NULL;
    r->status = NULL;
#else
    switch (timer) {
    case 0u:
        r->control = &REG_TIMER_0_CTRL;
        r->prescaler = &REG_TIMER_0_PSCR;
        r->counter = &REG_TIMER_0_CNT;
        r->compare = &REG_TIMER_0_CMP;
        r->status = &REG_TIMER_0_STAT;
        break;
    default:
        return ECOS_ERR_INVALID_ARGUMENT;
    }
#endif
    return ECOS_OK;
}

int hal_timer_get_instance_count(void)
{
    return LEGACY_TIMER_COUNT;
}

ecos_err_t hal_timer_init(hal_timer_id_t timer, const hal_timer_config_t *config)
{
    timer_registers_t r;
    int result;
    if (!timer_valid(timer) || config == NULL || config->period_ticks == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = timer_registers(timer, &r);
    if (result != ECOS_OK)
        return result;
#if LEGACY_TIMER_STYLE_C
    *r.control = TIMER_STOP;
    *r.legacy_data = config->period_ticks *
                     (uint32_t)(CONFIG_TIMER_FREQ_MHZ);
#else
    *r.control = TIMER_STOP;
    *r.prescaler = (uint32_t)(CONFIG_TIMER_FREQ_MHZ) - 1u;
    *r.compare = config->period_ticks - 1u;
#endif
    periods[timer] = config->period_ticks;
    initialized[timer] = 1u;
    return ECOS_OK;
}

ecos_err_t hal_timer_deinit(hal_timer_id_t timer)
{
    timer_registers_t r;
    int result;
    if (!timer_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    result = timer_registers(timer, &r);
    if (result != ECOS_OK)
        return result;
    *r.control = TIMER_STOP;
#if LEGACY_TIMER_STYLE_C
    *r.legacy_data = 0u;
#else
    *r.prescaler = 0u;
    *r.compare = 0u;
#endif
    initialized[timer] = 0u;
    periods[timer] = 0u;
    return ECOS_OK;
}

ecos_err_t hal_timer_start(hal_timer_id_t timer)
{
    timer_registers_t r;
    int result;
    if (!timer_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = timer_registers(timer, &r);
    if (result != ECOS_OK)
        return result;
    *r.control = TIMER_START;
    return ECOS_OK;
}

ecos_err_t hal_timer_stop(hal_timer_id_t timer)
{
    timer_registers_t r;
    int result;
    if (!timer_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = timer_registers(timer, &r);
    if (result != ECOS_OK)
        return result;
    *r.control = TIMER_STOP;
    return ECOS_OK;
}

ecos_err_t hal_timer_get_count(hal_timer_id_t timer, uint32_t *count)
{
    (void)timer;
    (void)count;
    return ECOS_ERR_UNSUPPORTED;
}

int hal_timer_is_expired(hal_timer_id_t timer)
{
    timer_registers_t r;
    int result;
    if (!timer_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = timer_registers(timer, &r);
    if (result != ECOS_OK)
        return result;
#if LEGACY_TIMER_STYLE_C
    return *r.counter == 0u ? TIMER_EXPIRED : 0;
#else
    return (*r.status & TIMER_EXPIRED) != 0u ? 1 : 0;
#endif
}

ecos_err_t hal_timer_delay_us(hal_timer_id_t timer, uint32_t duration_us)
{
    int result;
    if (!timer_valid(timer) || duration_us == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = hal_timer_init(timer, &(hal_timer_config_t){ duration_us });
    if (result != ECOS_OK)
        return result;
    result = hal_timer_start(timer);
    if (result != ECOS_OK)
        return result;
    do {
        result = hal_timer_is_expired(timer);
    } while (result == 0);
    if (result < 0)
        return result;
    return hal_timer_deinit(timer);
}
