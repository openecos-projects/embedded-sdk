#include "ecos/hal/pwm.h"

#include "hal_pwm.h"
#include "ysyx_2512_soc.h"

#include <stddef.h>
#include <stdint.h>

#define YSYX_2512_PWM_COUNT 2u
#define PWM_CHANNELS_PER_CONTROLLER 4u
#define PWM_GPIO1_PIN_BASE 14u
#define PWM_CONTROL_STOP 0u
#define PWM_CONTROL_START 3u

typedef struct {
    volatile uint32_t *control;
    volatile uint32_t *prescaler;
    volatile uint32_t *period;
    volatile uint32_t *duty[HAL_PWM_CHANNEL_COUNT];
} pwm_registers_t;

static uint8_t pwm_initialized[YSYX_2512_PWM_COUNT];
static uint32_t pwm_period_ticks[YSYX_2512_PWM_COUNT];

static int pwm_id_is_valid(hal_pwm_id_t pwm)
{
    return pwm < YSYX_2512_PWM_COUNT;
}

static int pwm_channel_is_valid(hal_pwm_channel_t channel)
{
    return channel >= HAL_PWM_CHANNEL_0 && channel < HAL_PWM_CHANNEL_COUNT;
}

static int pwm_get_registers(hal_pwm_id_t pwm, pwm_registers_t *registers)
{
    if (registers == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    switch (pwm) {
    case 0u:
        registers->control = &REG_PWM_0_CTRL;
        registers->prescaler = &REG_PWM_0_PSCR;
        registers->period = &REG_PWM_0_CMP;
        registers->duty[HAL_PWM_CHANNEL_0] = &REG_PWM_0_CR0;
        registers->duty[HAL_PWM_CHANNEL_1] = &REG_PWM_0_CR1;
        registers->duty[HAL_PWM_CHANNEL_2] = &REG_PWM_0_CR2;
        registers->duty[HAL_PWM_CHANNEL_3] = &REG_PWM_0_CR3;
        break;
    case 1u:
        registers->control = &REG_PWM_1_CTRL;
        registers->prescaler = &REG_PWM_1_PSCR;
        registers->period = &REG_PWM_1_CMP;
        registers->duty[HAL_PWM_CHANNEL_0] = &REG_PWM_1_CR0;
        registers->duty[HAL_PWM_CHANNEL_1] = &REG_PWM_1_CR1;
        registers->duty[HAL_PWM_CHANNEL_2] = &REG_PWM_1_CR2;
        registers->duty[HAL_PWM_CHANNEL_3] = &REG_PWM_1_CR3;
        break;
    default:
        return ECOS_ERR_INVALID_ARGUMENT;
    }
    return ECOS_OK;
}

static void pwm_configure_channel_pin(hal_pwm_id_t pwm,
                                      hal_pwm_channel_t channel)
{
    uint32_t pin = PWM_GPIO1_PIN_BASE +
                   ((uint32_t)pwm * PWM_CHANNELS_PER_CONTROLLER) +
                   (uint32_t)channel;
    uint32_t bit = (uint32_t)1u << pin;

    REG_GPIO_1_IOFCFG |= bit;
    REG_GPIO_1_PINMUX &= ~bit;
}

static void pwm_configure_all_pins(void)
{
    uint32_t pins = (uint32_t)0xFFu << PWM_GPIO1_PIN_BASE;

    REG_GPIO_1_IOFCFG |= pins;
    REG_GPIO_1_PINMUX &= ~pins;
}

static uint32_t pwm_scale_percent(uint32_t value, uint8_t percent)
{
    uint32_t quotient = 0u;
    uint32_t remainder = 0u;
    uint32_t scaled = 0u;
    uint32_t scaled_remainder = 0u;
    uint8_t bit = 32u;
    uint8_t index;

    /* Bitwise division avoids compiler runtime helpers on RV32E. */
    while (bit != 0u) {
        --bit;
        remainder = (remainder << 1) | ((value >> bit) & 1u);
        if (remainder >= 100u) {
            remainder -= 100u;
            quotient |= (uint32_t)1u << bit;
        }
    }

    for (index = 0u; index < percent; ++index) {
        scaled += quotient;
        scaled_remainder += remainder;
    }
    while (scaled_remainder >= 100u) {
        ++scaled;
        scaled_remainder -= 100u;
    }
    return scaled;
}

static ecos_err_t pwm_init_registers(hal_pwm_id_t pwm,
                                     uint32_t prescaler,
                                     uint32_t period,
                                     uint32_t initial_duty)
{
    pwm_registers_t registers;
    uint8_t channel;
    int result = pwm_get_registers(pwm, &registers);

    if (result != ECOS_OK)
        return result;

    *registers.control = PWM_CONTROL_STOP;
    *registers.prescaler = prescaler;
    *registers.period = period;
    for (channel = 0u; channel < HAL_PWM_CHANNEL_COUNT; ++channel)
        *registers.duty[channel] = initial_duty;
    pwm_initialized[pwm] = 1u;
    return ECOS_OK;
}

int hal_pwm_get_instance_count(void)
{
    return (int)YSYX_2512_PWM_COUNT;
}

ecos_err_t hal_pwm_init(hal_pwm_id_t pwm, const hal_pwm_config_t *config)
{
    int result;

    if (!pwm_id_is_valid(pwm) || config == NULL ||
        config->clock_divider == 0u || config->period_ticks == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    result = pwm_init_registers(
        pwm, config->clock_divider - 1u, config->period_ticks - 1u, 0u
    );
    if (result == ECOS_OK)
        pwm_period_ticks[pwm] = config->period_ticks;
    return result;
}

ecos_err_t hal_pwm_set_duty_cycle(hal_pwm_id_t pwm,
                                  hal_pwm_channel_t channel,
                                  uint8_t duty_percent)
{
    pwm_registers_t registers;
    int result;

    if (!pwm_id_is_valid(pwm) || !pwm_channel_is_valid(channel) ||
        duty_percent > 100u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (pwm_initialized[pwm] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    result = pwm_get_registers(pwm, &registers);
    if (result != ECOS_OK)
        return result;
    pwm_configure_channel_pin(pwm, channel);
    *registers.duty[channel] = pwm_scale_percent(
        pwm_period_ticks[pwm], duty_percent
    );
    return ECOS_OK;
}

ecos_err_t hal_pwm_start(hal_pwm_id_t pwm)
{
    pwm_registers_t registers;
    int result;

    if (!pwm_id_is_valid(pwm))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (pwm_initialized[pwm] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    result = pwm_get_registers(pwm, &registers);
    if (result != ECOS_OK)
        return result;
    *registers.control = PWM_CONTROL_START;
    return ECOS_OK;
}

ecos_err_t hal_pwm_stop(hal_pwm_id_t pwm)
{
    pwm_registers_t registers;
    int result;

    if (!pwm_id_is_valid(pwm))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (pwm_initialized[pwm] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    result = pwm_get_registers(pwm, &registers);
    if (result != ECOS_OK)
        return result;
    *registers.control = PWM_CONTROL_STOP;
    return ECOS_OK;
}

/* SDK 2.x compatibility wrappers for existing L4 templates and devices. */
uint8_t pwm_hal_init(void *hal, uint8_t timer_id, pwm_config_t *config)
{
    ecos_err_t result;

    (void)hal;
    if (config == NULL || !pwm_id_is_valid((hal_pwm_id_t)timer_id))
        return 1u;

    pwm_configure_all_pins();
    result = pwm_init_registers(
        (hal_pwm_id_t)timer_id, config->pscr, config->cmp, config->cmp
    );
    if (result == ECOS_OK)
        pwm_period_ticks[timer_id] = config->cmp;
    return result == ECOS_OK ? 0u : 1u;
}

uint8_t pwm_hal_set_compare(void *hal,
                            uint8_t timer_id,
                            pwm_channel_t channel,
                            uint32_t compare)
{
    pwm_registers_t registers;
    int result;

    (void)hal;
    if (!pwm_id_is_valid((hal_pwm_id_t)timer_id) ||
        channel < PWM_CH0 || channel >= PWM_CH_MAX)
        return 1u;

    result = pwm_get_registers((hal_pwm_id_t)timer_id, &registers);
    if (result != ECOS_OK)
        return 1u;
    *registers.duty[channel] = compare;
    return 0u;
}

uint8_t pwm_hal_enable(void *hal, uint8_t timer_id)
{
    (void)hal;
    return hal_pwm_start((hal_pwm_id_t)timer_id) == ECOS_OK ? 0u : 1u;
}

uint8_t pwm_hal_disable(void *hal, uint8_t timer_id)
{
    (void)hal;
    return hal_pwm_stop((hal_pwm_id_t)timer_id) == ECOS_OK ? 0u : 1u;
}
