#include "ecos/hal/timer.h"
#include "ysyx_2512_soc.h"

#include <stddef.h>
#include <stdint.h>

#ifndef CONFIG_TIMER_FREQ_MHZ
#define CONFIG_TIMER_FREQ_MHZ 25u
#endif

#define TIMER_CONTROL_STOP  0x00u
#define TIMER_CONTROL_START 0x0Du
#define YSYX_2512_TIMER_COUNT 4u
#define TIMER_MAX_DELAY_MS 4294967u

typedef struct {
    volatile uint32_t *control;
    volatile uint32_t *prescaler;
    volatile uint32_t *compare;
    volatile uint32_t *status;
} timer_registers_t;

static uint8_t timer_initialized[YSYX_2512_TIMER_COUNT];

static int timer_id_is_valid(hal_timer_id_t timer)
{
    return timer < YSYX_2512_TIMER_COUNT;
}

static int timer_get_registers(hal_timer_id_t timer,
                               timer_registers_t *registers)
{
    if (registers == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    switch (timer) {
    case 0u:
        registers->control = &REG_TIMER_0_CTRL;
        registers->prescaler = &REG_TIMER_0_PSCR;
        registers->compare = &REG_TIMER_0_CMP;
        registers->status = &REG_TIMER_0_STAT;
        break;
    case 1u:
        registers->control = &REG_TIMER_1_CTRL;
        registers->prescaler = &REG_TIMER_1_PSCR;
        registers->compare = &REG_TIMER_1_CMP;
        registers->status = &REG_TIMER_1_STAT;
        break;
    case 2u:
        registers->control = &REG_TIMER_2_CTRL;
        registers->prescaler = &REG_TIMER_2_PSCR;
        registers->compare = &REG_TIMER_2_CMP;
        registers->status = &REG_TIMER_2_STAT;
        break;
    case 3u:
        registers->control = &REG_TIMER_3_CTRL;
        registers->prescaler = &REG_TIMER_3_PSCR;
        registers->compare = &REG_TIMER_3_CMP;
        registers->status = &REG_TIMER_3_STAT;
        break;
    default:
        return ECOS_ERR_INVALID_ARGUMENT;
    }
    return ECOS_OK;
}

static void timer_stop_and_clear(const timer_registers_t *registers)
{
    *registers->control = TIMER_CONTROL_STOP;
    /* L4 clears the latched overflow flag when STAT is read while stopped. */
    while (*registers->status != 0u)
        ;
}

int hal_timer_get_instance_count(void)
{
    return (int)YSYX_2512_TIMER_COUNT;
}

ecos_err_t hal_timer_init(hal_timer_id_t timer,
                          const hal_timer_config_t *config)
{
    timer_registers_t registers;
    int result;

    if (!timer_id_is_valid(timer) || config == NULL ||
        config->period_ticks == 0u || CONFIG_TIMER_FREQ_MHZ == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    result = timer_get_registers(timer, &registers);
    if (result != ECOS_OK)
        return result;

    timer_stop_and_clear(&registers);
    *registers.prescaler = (uint32_t)CONFIG_TIMER_FREQ_MHZ - 1u;
    *registers.compare = config->period_ticks - 1u;
    timer_initialized[timer] = 1u;
    return ECOS_OK;
}

ecos_err_t hal_timer_deinit(hal_timer_id_t timer)
{
    timer_registers_t registers;
    int result;

    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;

    result = timer_get_registers(timer, &registers);
    if (result != ECOS_OK)
        return result;
    timer_stop_and_clear(&registers);
    *registers.prescaler = 0u;
    *registers.compare = 0u;
    timer_initialized[timer] = 0u;
    return ECOS_OK;
}

ecos_err_t hal_timer_start(hal_timer_id_t timer)
{
    timer_registers_t registers;
    int result;

    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    result = timer_get_registers(timer, &registers);
    if (result != ECOS_OK)
        return result;
    timer_stop_and_clear(&registers);
    *registers.control = TIMER_CONTROL_START;
    return ECOS_OK;
}

ecos_err_t hal_timer_stop(hal_timer_id_t timer)
{
    timer_registers_t registers;
    int result;

    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    result = timer_get_registers(timer, &registers);
    if (result != ECOS_OK)
        return result;
    timer_stop_and_clear(&registers);
    return ECOS_OK;
}

ecos_err_t hal_timer_get_count(hal_timer_id_t timer, uint32_t *count)
{
    if (!timer_id_is_valid(timer) || count == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_ERR_UNSUPPORTED;
}

int hal_timer_is_expired(hal_timer_id_t timer)
{
    timer_registers_t registers;
    int result;

    if (!timer_id_is_valid(timer))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (timer_initialized[timer] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    result = timer_get_registers(timer, &registers);
    if (result != ECOS_OK)
        return result;
    return *registers.status != 0u ? 1 : 0;
}

ecos_err_t hal_timer_delay_us(hal_timer_id_t timer, uint32_t duration_us)
{
    const hal_timer_config_t config = { duration_us };
    int result;

    if (!timer_id_is_valid(timer) || duration_us == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

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
    return hal_timer_deinit(timer);
}

/* SDK 2.x 兼容包装，供现有 L4 模板和 Device Driver 使用。 */
uint8_t hal_delay_us(uint8_t timer_id, uint32_t value)
{
    return hal_timer_delay_us((hal_timer_id_t)timer_id, value) == ECOS_OK ?
           0u : 1u;
}

uint8_t hal_delay_ms(uint8_t timer_id, uint32_t value)
{
    hal_timer_id_t timer = (hal_timer_id_t)timer_id;

    if (!timer_id_is_valid(timer))
        return 1u;

    while (value != 0u) {
        uint32_t current = value;

        if (current > TIMER_MAX_DELAY_MS)
            current = TIMER_MAX_DELAY_MS;
        if (hal_timer_delay_us(timer,
                               (current << 10) - (current << 4) -
                               (current << 3)) != ECOS_OK)
            return 1u;
        value -= current;
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
    const hal_timer_config_t config = { UINT32_MAX };
    hal_timer_id_t timer = (hal_timer_id_t)timer_id;

    if (hal_timer_init(timer, &config) != ECOS_OK)
        return 1u;
    return hal_timer_start(timer) == ECOS_OK ? 0u : 1u;
}

uint32_t hal_get_sys_tick(uint8_t timer_id)
{
    uint32_t count;

    if (hal_timer_get_count((hal_timer_id_t)timer_id, &count) != ECOS_OK)
        return 0u;
    return count;
}

int hal_timer_clear_interrupt(uint8_t timer_id)
{
    return hal_timer_stop((hal_timer_id_t)timer_id) == ECOS_OK ? 0 : -1;
}

int hal_timer_register_callback(uint8_t timer_id,
                                void (*callback)(void *),
                                void *arg,
                                uint32_t priority)
{
    (void)timer_id;
    (void)callback;
    (void)arg;
    (void)priority;
    return ECOS_ERR_UNSUPPORTED;
}

int hal_timer_unregister_callback(uint8_t timer_id)
{
    (void)timer_id;
    return ECOS_ERR_UNSUPPORTED;
}

__attribute__((weak)) void delay_ms(uint32_t value)
{
    (void)hal_delay_ms(0u, value);
}
