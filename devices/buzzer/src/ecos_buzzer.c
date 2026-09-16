#include "ecos/device/buzzer.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define BUZZER_TRY(expression)                   \
    do {                                         \
        ecos_err_t buzzer_result = (expression); \
        if (buzzer_result != ECOS_OK)            \
            return buzzer_result;                \
    } while (0)

#define BUZZER_DUTY_PERCENT 50u
/* Largest period the tone divider picks; keeps the duty resolution at
 * 12 bits or better across the audible range. */
#define BUZZER_MAX_PERIOD_TICKS 4096u

/* The rv32e runtime links without libgcc division helpers, so the tone
 * timings are computed with shift-subtract long division. */
static uint32_t buzzer_divide(uint32_t numerator, uint32_t denominator)
{
    uint32_t quotient = 0u;
    uint32_t remainder = 0u;
    int bit;

    for (bit = 31; bit >= 0; --bit) {
        remainder = (remainder << 1) | ((numerator >> bit) & 1u);
        if (remainder >= denominator) {
            remainder -= denominator;
            quotient |= (uint32_t)1u << bit;
        }
    }
    return quotient;
}

static ecos_err_t buzzer_tone_config(const ecos_buzzer_config_t *config,
                                     uint32_t frequency_hz,
                                     ecos_pwm_config_t *pwm_config)
{
    uint32_t total_ticks;
    uint32_t divider;
    uint32_t period_ticks;

    if (frequency_hz == 0u || frequency_hz > config->pwm_clock_hz)
        return ECOS_ERR_INVALID_ARGUMENT;

    total_ticks = buzzer_divide(config->pwm_clock_hz, frequency_hz);
    if (total_ticks == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    divider = (total_ticks + BUZZER_MAX_PERIOD_TICKS - 1u) >>
              12u; /* ceil(total / BUZZER_MAX_PERIOD_TICKS) */
    if (divider == 0u)
        divider = 1u;
    period_ticks = buzzer_divide(total_ticks, divider);
    if (period_ticks == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    pwm_config->clock_divider = divider;
    pwm_config->period_ticks = period_ticks;
    return ECOS_OK;
}

ecos_err_t ecos_buzzer_init(ecos_buzzer_t *buzzer,
                            const ecos_buzzer_config_t *config)
{
    int instance_count;

    if (buzzer == NULL || config == NULL || config->pwm_clock_hz == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    instance_count = ecos_pwm_get_instance_count();
    if (instance_count < 0)
        return (ecos_err_t)instance_count;
    if (instance_count <= (int)config->pwm)
        return ECOS_ERR_NOT_FOUND;

    buzzer->config = *config;
    buzzer->initialized = 1u;
    return ECOS_OK;
}

ecos_err_t ecos_buzzer_deinit(ecos_buzzer_t *buzzer)
{
    if (buzzer == NULL || buzzer->initialized == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    BUZZER_TRY(ecos_pwm_stop(buzzer->config.pwm));
    buzzer->initialized = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_buzzer_play_tone(ecos_buzzer_t *buzzer, uint32_t frequency_hz)
{
    ecos_pwm_config_t pwm_config;

    if (buzzer == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (buzzer->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    BUZZER_TRY(buzzer_tone_config(&buzzer->config, frequency_hz,
                                  &pwm_config));
    BUZZER_TRY(ecos_pwm_init(buzzer->config.pwm, &pwm_config));
    BUZZER_TRY(ecos_pwm_set_duty_cycle(buzzer->config.pwm,
                                       buzzer->config.channel,
                                       BUZZER_DUTY_PERCENT));
    return ecos_pwm_start(buzzer->config.pwm);
}

ecos_err_t ecos_buzzer_beep(ecos_buzzer_t *buzzer,
                            uint32_t frequency_hz,
                            uint32_t duration_ms)
{
    if (buzzer == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (buzzer->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    BUZZER_TRY(ecos_buzzer_play_tone(buzzer, frequency_hz));
    BUZZER_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, duration_ms));
    return ecos_buzzer_stop(buzzer);
}

ecos_err_t ecos_buzzer_stop(ecos_buzzer_t *buzzer)
{
    if (buzzer == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (buzzer->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    return ecos_pwm_stop(buzzer->config.pwm);
}
