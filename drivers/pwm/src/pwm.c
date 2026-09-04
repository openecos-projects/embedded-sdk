#include "ecos/driver/pwm.h"

#include "ecos/hal/pwm.h"

#include <stddef.h>

static int pwm_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

int ecos_pwm_get_instance_count(void)
{
    return pwm_map_hal_result(hal_pwm_get_instance_count());
}

ecos_err_t ecos_pwm_init(ecos_pwm_id_t pwm,
                         const ecos_pwm_config_t *config)
{
    hal_pwm_config_t hal_config;

    if (config == NULL || config->clock_divider == 0u ||
        config->period_ticks == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.clock_divider = config->clock_divider;
    hal_config.period_ticks = config->period_ticks;
    return pwm_map_hal_result(
        hal_pwm_init((hal_pwm_id_t)pwm, &hal_config)
    );
}

ecos_err_t ecos_pwm_set_duty_cycle(ecos_pwm_id_t pwm,
                                   ecos_pwm_channel_t channel,
                                   uint8_t duty_percent)
{
    if (channel < ECOS_PWM_CHANNEL_0 || channel >= ECOS_PWM_CHANNEL_COUNT ||
        duty_percent > 100u)
        return ECOS_ERR_INVALID_ARGUMENT;

    return pwm_map_hal_result(
        hal_pwm_set_duty_cycle(
            (hal_pwm_id_t)pwm,
            (hal_pwm_channel_t)channel,
            duty_percent
        )
    );
}

ecos_err_t ecos_pwm_start(ecos_pwm_id_t pwm)
{
    return pwm_map_hal_result(hal_pwm_start((hal_pwm_id_t)pwm));
}

ecos_err_t ecos_pwm_stop(ecos_pwm_id_t pwm)
{
    return pwm_map_hal_result(hal_pwm_stop((hal_pwm_id_t)pwm));
}
