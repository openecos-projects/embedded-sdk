/**
 * @file pwm_driver.c
 * @brief StarrySky C2 PWM driver implementation
 */

#include "pwm.h"
#include "board.h"  // Include C2 board header for register definitions

// Static function declarations
static status_t pwm_init_impl(const pwm_config_t* config);
static status_t pwm_set_compare_impl(pwm_channel_t ch, uint32_t cmp);

// PWM driver instance for StarrySky C2
static pwm_driver_t starrysky_c2_pwm_driver = {
    .base = {
        .version = DRIVER_VERSION(1, 0),
        .name = "starrysky_c2_pwm"
    },
    .init = pwm_init_impl,
    .set_compare = pwm_set_compare_impl
};

status_t starrysky_c2_pwm_init(void)
{
    return pwm_register_driver(&starrysky_c2_pwm_driver);
}

static status_t pwm_init_impl(const pwm_config_t* config)
{
    CHECK_NULL(config);
    
    // Set PWM clock prescaler
    REG_PWM_0_PSCR = config->pscr;
    
    // Set PWM compare value
    REG_PWM_0_CMP = config->cmp;
    
    // Enable PWM (set control register bit 0)
    REG_PWM_0_CTRL |= 0x1;
    
    return STATUS_SUCCESS;
}

static status_t pwm_set_compare_impl(pwm_channel_t ch, uint32_t cmp)
{
    // Validate channel number (C2 has PWM channels 0-3)
    if (ch > PWM_CH3) {
        return STATUS_INVALID_ARG;
    }
    
    // Set compare value for the specified channel
    switch (ch) {
        case PWM_CH0:
            REG_PWM_0_CR0 = cmp;
            break;
        case PWM_CH1:
            REG_PWM_0_CR1 = cmp;
            break;
        case PWM_CH2:
            REG_PWM_0_CR2 = cmp;
            break;
        case PWM_CH3:
            REG_PWM_0_CR3 = cmp;
            break;
        default:
            return STATUS_INVALID_ARG;
    }
    
    return STATUS_SUCCESS;
}