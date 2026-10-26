/* CH32V103 TIM1 channel 1 PWM backend. Default output is PA8. */
#include "ecos/hal/pwm.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_APB2PCENR  REG32(0x40021018u)
#define GPIOA_CFGHR    REG32(0x40010804u)
#define TIM1_BASE      0x40012C00u
#define TIM1_CTLR1     REG16(TIM1_BASE + 0x00u)
#define TIM1_SWEVGR    REG16(TIM1_BASE + 0x14u)
#define TIM1_CHCTLR1   REG16(TIM1_BASE + 0x18u)
#define TIM1_CCER      REG16(TIM1_BASE + 0x20u)
#define TIM1_PSC       REG16(TIM1_BASE + 0x28u)
#define TIM1_ATRLR     REG16(TIM1_BASE + 0x2Cu)
#define TIM1_CH1CVR    REG16(TIM1_BASE + 0x34u)
#define TIM1_BDTR      REG16(TIM1_BASE + 0x44u)
#define TIM_CEN        0x0001u
#define TIM_ARPE       0x0080u
#define TIM_CC1E       0x0001u
#define TIM_MOE        0x8000u
#define TIM_OC1M_PWM1  0x0060u
#define TIM_OC1PE      0x0008u

static uint8_t initialized;
static uint32_t period_ticks;

static int valid(hal_pwm_id_t id, hal_pwm_channel_t channel)
{
    return id == 0u && channel == HAL_PWM_CHANNEL_0;
}

int hal_pwm_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_pwm_init(hal_pwm_id_t id, const hal_pwm_config_t *config)
{
    if (id != 0u || config == NULL || config->clock_divider == 0u ||
        config->period_ticks == 0u || config->period_ticks > 65536u ||
        config->clock_divider > 65536u)
        return ECOS_ERR_INVALID_ARGUMENT;

    RCC_APB2PCENR |= (1u << 2) | (1u << 11); /* GPIOA and TIM1 */
    /* PA8 alternate-function push-pull, 50 MHz: CFGHR nibble 0xB. */
    GPIOA_CFGHR = (GPIOA_CFGHR & ~0xFu) | 0xBu;
    TIM1_CTLR1 = 0u;
    TIM1_PSC = (uint16_t)(config->clock_divider - 1u);
    TIM1_ATRLR = (uint16_t)(config->period_ticks - 1u);
    TIM1_CHCTLR1 = TIM_OC1M_PWM1 | TIM_OC1PE;
    TIM1_CCER = TIM_CC1E;
    TIM1_BDTR = TIM_MOE;
    TIM1_CH1CVR = 0u;
    TIM1_CTLR1 = TIM_ARPE;
    TIM1_SWEVGR = 1u;
    period_ticks = config->period_ticks;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_pwm_set_duty_cycle(hal_pwm_id_t id,
                                  hal_pwm_channel_t channel,
                                  uint8_t duty_percent)
{
    if (!valid(id, channel) || duty_percent > 100u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    TIM1_CH1CVR = (uint16_t)((period_ticks * duty_percent) / 100u);
    return ECOS_OK;
}

ecos_err_t hal_pwm_start(hal_pwm_id_t id)
{
    if (id != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    TIM1_CTLR1 |= TIM_CEN;
    return ECOS_OK;
}

ecos_err_t hal_pwm_stop(hal_pwm_id_t id)
{
    if (id != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    TIM1_CTLR1 &= (uint16_t)~TIM_CEN;
    return ECOS_OK;
}
