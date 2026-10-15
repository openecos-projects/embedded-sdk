#include "ecos/hal/pwm.h"
#include "legacy_soc.h"

#include <stddef.h>

#define PWM_STOP 0u
#define PWM_START 3u

typedef struct {
    volatile uint32_t *control;
    volatile uint32_t *prescaler;
    volatile uint32_t *period;
    volatile uint32_t *duty[HAL_PWM_CHANNEL_COUNT];
} pwm_registers_t;

static uint8_t initialized[3];
static uint32_t periods[3];

static int pwm_valid(hal_pwm_id_t pwm)
{
    return pwm < LEGACY_PWM_COUNT;
}

static int pwm_registers(hal_pwm_id_t pwm, pwm_registers_t *r)
{
    if (r == NULL || !pwm_valid(pwm))
        return ECOS_ERR_INVALID_ARGUMENT;
    switch (pwm) {
    case 0u:
        r->control = &REG_PWM_0_CTRL;
        r->prescaler = &REG_PWM_0_PSCR;
        r->period = &REG_PWM_0_CMP;
        r->duty[0] = &REG_PWM_0_CR0;
        r->duty[1] = &REG_PWM_0_CR1;
        r->duty[2] = &REG_PWM_0_CR2;
        r->duty[3] = &REG_PWM_0_CR3;
        break;
#if LEGACY_PWM_COUNT > 1
    case 1u:
        r->control = &REG_PWM_1_CTRL;
        r->prescaler = &REG_PWM_1_PSCR;
        r->period = &REG_PWM_1_CMP;
        r->duty[0] = &REG_PWM_1_CR0;
        r->duty[1] = &REG_PWM_1_CR1;
        r->duty[2] = &REG_PWM_1_CR2;
        r->duty[3] = &REG_PWM_1_CR3;
        break;
#endif
#if LEGACY_PWM_COUNT > 2
    case 2u:
        r->control = &REG_PWM_2_CTRL;
        r->prescaler = &REG_PWM_2_PSCR;
        r->period = &REG_PWM_2_CMP;
        r->duty[0] = &REG_PWM_2_CR0;
        r->duty[1] = &REG_PWM_2_CR1;
        r->duty[2] = &REG_PWM_2_CR2;
        r->duty[3] = &REG_PWM_2_CR3;
        break;
#endif
    default:
        return ECOS_ERR_INVALID_ARGUMENT;
    }
    return ECOS_OK;
}

static uint32_t divide_by_100(uint32_t value, uint32_t *remainder)
{
    uint32_t quotient = 0u;
    uint32_t rest = 0u;
    int bit;

    for (bit = 31; bit >= 0; --bit) {
        rest = (rest << 1) | ((value >> bit) & 1u);
        if (rest >= 100u) {
            rest -= 100u;
            quotient |= (uint32_t)1u << bit;
        }
    }
    if (remainder != NULL)
        *remainder = rest;
    return quotient;
}

static uint32_t scale_percent(uint32_t value, uint8_t percent)
{
    uint32_t remainder;
    uint32_t quotient = divide_by_100(value, &remainder);
    uint32_t scaled = 0u;
    uint32_t scaled_remainder = 0u;
    uint8_t step;

    for (step = 0u; step < percent; ++step) {
        scaled += quotient;
        scaled_remainder += remainder;
        if (scaled_remainder >= 100u) {
            ++scaled;
            scaled_remainder -= 100u;
        }
    }
    return scaled;
}

static void configure_pin(hal_pwm_id_t pwm, hal_pwm_channel_t channel)
{
#if LEGACY_HAS_GPIO_MUX
    uint32_t pin = 14u + (uint32_t)pwm * 4u + (uint32_t)channel;
    uint32_t bit = (uint32_t)1u << pin;
    REG_GPIO_1_IOFCFG |= bit;
    REG_GPIO_1_PINMUX &= ~bit;
#else
    (void)pwm;
    (void)channel;
#endif
}

int hal_pwm_get_instance_count(void)
{
    return LEGACY_PWM_COUNT;
}

ecos_err_t hal_pwm_init(hal_pwm_id_t pwm, const hal_pwm_config_t *config)
{
    pwm_registers_t r;
    int result;
    uint8_t channel;
    if (!pwm_valid(pwm) || config == NULL || config->clock_divider == 0u ||
        config->period_ticks == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = pwm_registers(pwm, &r);
    if (result != ECOS_OK)
        return result;
    *r.control = PWM_STOP;
    *r.prescaler = config->clock_divider - 1u;
    *r.period = config->period_ticks - 1u;
    for (channel = 0u; channel < HAL_PWM_CHANNEL_COUNT; ++channel)
        *r.duty[channel] = 0u;
    periods[pwm] = config->period_ticks;
    initialized[pwm] = 1u;
    return ECOS_OK;
}

ecos_err_t hal_pwm_set_duty_cycle(hal_pwm_id_t pwm, hal_pwm_channel_t channel,
                                  uint8_t duty_percent)
{
    pwm_registers_t r;
    int result;
    if (!pwm_valid(pwm) || channel >= HAL_PWM_CHANNEL_COUNT || duty_percent > 100u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[pwm] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = pwm_registers(pwm, &r);
    if (result != ECOS_OK)
        return result;
    configure_pin(pwm, channel);
    *r.duty[channel] = scale_percent(periods[pwm], duty_percent);
    return ECOS_OK;
}

ecos_err_t hal_pwm_start(hal_pwm_id_t pwm)
{
    pwm_registers_t r;
    int result;
    if (!pwm_valid(pwm) || initialized[pwm] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = pwm_registers(pwm, &r);
    if (result != ECOS_OK)
        return result;
    *r.control = PWM_START;
    return ECOS_OK;
}

ecos_err_t hal_pwm_stop(hal_pwm_id_t pwm)
{
    pwm_registers_t r;
    int result;
    if (!pwm_valid(pwm) || initialized[pwm] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = pwm_registers(pwm, &r);
    if (result != ECOS_OK)
        return result;
    *r.control = PWM_STOP;
    return ECOS_OK;
}
