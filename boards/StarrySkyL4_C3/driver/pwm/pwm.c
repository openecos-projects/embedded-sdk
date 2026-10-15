#include "hal_pwm.h"
#include "hal_pwm_type.h"
#include "board.h"
#include <stdint.h>

#define PWM_GPIO1_PIN_BASE      14u
#define PWM_GPIO1_PIN_COUNT     8u
#define PWM_GPIO1_PINS          (0xFFu << PWM_GPIO1_PIN_BASE)

static void pwm_gpio_init(void)
{
    /*
     * L4 PWM pin mux:
     *   GPIO1[14:17] -> PWM0.D0..D3
     *   GPIO1[18:21] -> PWM1.D0..D3
     * Alternate function channel 0.
     */
    REG_GPIO_1_IOFCFG |= PWM_GPIO1_PINS;
    REG_GPIO_1_PINMUX &= ~PWM_GPIO1_PINS;
}

uint8_t pwm_hal_init(void *hal, uint8_t timer_id, pwm_config_t *config){
    (void)hal;

    if (config == 0) {
        return 1;
    }

    pwm_gpio_init();

    if (timer_id == 0)
    {
        REG_PWM_0_CTRL = 0;
        REG_PWM_0_PSCR = config->pscr;
        REG_PWM_0_CMP  = config->cmp;
        REG_PWM_0_CR0 = config->cmp;
        REG_PWM_0_CR1 = config->cmp;
        REG_PWM_0_CR2 = config->cmp;
        REG_PWM_0_CR3 = config->cmp;
    }
    else if (timer_id == 1)
    {
        REG_PWM_1_CTRL = 0;
        REG_PWM_1_PSCR = config->pscr;
        REG_PWM_1_CMP  = config->cmp;
        REG_PWM_1_CR0 = config->cmp;
        REG_PWM_1_CR1 = config->cmp;
        REG_PWM_1_CR2 = config->cmp;
        REG_PWM_1_CR3 = config->cmp;
    }
    else if (timer_id == 2)
    {
#ifdef REG_PWM_2_CTRL
        REG_PWM_2_CTRL = 0;
        REG_PWM_2_PSCR = config->pscr;
        REG_PWM_2_CMP  = config->cmp;
#else
        return 1;
#endif
    } else {
        return 1;
    }
    return 0;
}

uint8_t pwm_hal_set_compare(void *hal, uint8_t timer_id, pwm_channel_t ch, uint32_t cmp){
    (void)hal;

    if (timer_id == 0)
    {
        switch(ch)
        {
            case PWM_CH0: REG_PWM_0_CR0 = cmp; break;
            case PWM_CH1: REG_PWM_0_CR1 = cmp; break;
            case PWM_CH2: REG_PWM_0_CR2 = cmp; break;
            case PWM_CH3: REG_PWM_0_CR3 = cmp; break;
            default: break;
        }
    }
    else if (timer_id == 1)
    {
        switch(ch)
        {
            case PWM_CH0: REG_PWM_1_CR0 = cmp; break;
            case PWM_CH1: REG_PWM_1_CR1 = cmp; break;
            case PWM_CH2: REG_PWM_1_CR2 = cmp; break;
            case PWM_CH3: REG_PWM_1_CR3 = cmp; break;
            default: break;
        }
    }
    else if (timer_id == 2)
    {
#ifdef REG_PWM_2_CTRL
        switch(ch)
        {
            case PWM_CH0: REG_PWM_2_CR0 = cmp; break;
            case PWM_CH1: REG_PWM_2_CR1 = cmp; break;
            case PWM_CH2: REG_PWM_2_CR2 = cmp; break;
            case PWM_CH3: REG_PWM_2_CR3 = cmp; break;
            default: break;
        }
#else
        return 1;
#endif
    } else {
        return 1;
    }
    return 0;
}

uint8_t pwm_hal_enable(void *hal, uint8_t timer_id){
    (void)hal;

    if (timer_id == 0)
    {
        REG_PWM_0_CTRL = 3;
    }
    else if (timer_id == 1)
    {
        REG_PWM_1_CTRL = 3;
    }
    else if (timer_id == 2)
    {
#ifdef REG_PWM_2_CTRL
        REG_PWM_2_CTRL = 3;
#else
        return 1;
#endif
    } else {
        return 1;
    }
    return 0;
}

uint8_t pwm_hal_disable(void *hal, uint8_t timer_id){
    (void)hal;

    if (timer_id == 0)
    {
        REG_PWM_0_CTRL = 0;
    }
    else if (timer_id == 1)
    {
        REG_PWM_1_CTRL = 0;
    }
    else if (timer_id == 2)
    {
#ifdef REG_PWM_2_CTRL
        REG_PWM_2_CTRL = 0;
#else
        return 1;
#endif
    } else {
        return 1;
    }
    return 0;
}
