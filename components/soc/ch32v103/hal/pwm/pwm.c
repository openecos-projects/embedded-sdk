/* PWM0 uses TIM1_CH1 on PA8. PWM1 uses partially remapped TIM3 on PB4/PB5/PB0/PB1. */
#include "ecos/hal/pwm.h"
#include "ch32v103_interrupt_internal.h"

#include <stdbool.h>
#include <stddef.h>

#define RCC_APB2PCENR  0x40021018u
#define RCC_APB1PCENR  0x4002101Cu
#define AFIO_PCFR1     0x40010004u
#define GPIOA_CFGHR    0x40010804u
#define GPIOB_CFGLR    0x40010C00u
#define TIM1_BASE      0x40012C00u
#define TIM3_BASE      0x40000400u

#define TIM_CTLR1     0x00u
#define TIM_SWEVGR    0x14u
#define TIM_CHCTLR1   0x18u
#define TIM_CHCTLR2   0x1Cu
#define TIM_CCER      0x20u
#define TIM_PSC       0x28u
#define TIM_ATRLR     0x2Cu
#define TIM_CH1CVR    0x34u
#define TIM_BDTR      0x44u

#define TIM_CEN       0x0001u
#define TIM_ARPE      0x0080u
#define TIM_UG        0x0001u
#define TIM_MOE       0x8000u
#define TIM_OC_MODE   0x70u
#define TIM_OC_PWM1   0x60u
#define TIM_OC_ACTIVE 0x50u
#define TIM_OC_INACTIVE 0x40u
#define TIM_OC_PE     0x08u
#define TIM_CC1E      0x0001u
#define TIM_CC1E_TO_CC4E 0x1111u

#define PWM_INSTANCE_COUNT 2u

static const uintptr_t timer_base[PWM_INSTANCE_COUNT] = {TIM1_BASE, TIM3_BASE};
/* Zero marks an instance that has never been initialized. */
static uint32_t period_ticks[PWM_INSTANCE_COUNT];

int hal_pwm_get_instance_count(void)
{
    return PWM_INSTANCE_COUNT;
}

static void configure_pins_and_clocks(hal_pwm_id_t id)
{
    uint32_t clocks = ch32v103_read32(RCC_APB2PCENR);
    if (id == 0u) {
        ch32v103_write32(RCC_APB2PCENR, clocks | (1u << 2) | (1u << 11)); /* GPIOA, TIM1 */
        ch32v103_write32(GPIOA_CFGHR, (ch32v103_read32(GPIOA_CFGHR) & ~0xFu) | 0xBu);
    } else {
        ch32v103_write32(RCC_APB2PCENR, clocks | (1u << 0) | (1u << 3)); /* AFIO, GPIOB */
        ch32v103_write32(RCC_APB1PCENR, ch32v103_read32(RCC_APB1PCENR) | (1u << 1)); /* TIM3 */
        uint32_t remap = ch32v103_read32(AFIO_PCFR1);
        ch32v103_write32(AFIO_PCFR1, (remap & ~(3u << 10)) | (2u << 10));
        /* Only PB0, PB1, PB4 and PB5 become alternate-function push-pull. */
        uint32_t pins = ch32v103_read32(GPIOB_CFGLR);
        ch32v103_write32(GPIOB_CFGLR, (pins & ~0x00FF00FFu) | 0x00BB00BBu);
    }
}

ecos_err_t hal_pwm_init(hal_pwm_id_t id, const hal_pwm_config_t *config)
{
    if (id >= PWM_INSTANCE_COUNT || config == NULL ||
        config->clock_divider == 0u || config->clock_divider > 65536u ||
        config->period_ticks == 0u || config->period_ticks > 65536u)
        return ECOS_ERR_INVALID_ARGUMENT;

    uintptr_t base = timer_base[id];
    uint32_t state = ch32v103_irq_lock();
    configure_pins_and_clocks(id);
    ch32v103_write16(base + TIM_CCER, 0u);
    ch32v103_write16(base + TIM_CTLR1, TIM_ARPE);
    ch32v103_write16(base + TIM_PSC, (uint16_t)(config->clock_divider - 1u));
    ch32v103_write16(base + TIM_ATRLR, (uint16_t)(config->period_ticks - 1u));
    uint16_t mode = TIM_OC_PWM1 | TIM_OC_PE;
    if (id == 0u) {
        ch32v103_write16(base + TIM_CHCTLR1, mode);
        ch32v103_write16(base + TIM_BDTR, TIM_MOE);
    } else {
        uint16_t pair = mode | (mode << 8);
        ch32v103_write16(base + TIM_CHCTLR1, pair);
        ch32v103_write16(base + TIM_CHCTLR2, pair);
    }
    unsigned channels = id == 0u ? 1u : HAL_PWM_CHANNEL_COUNT;
    for (unsigned channel = 0; channel < channels; ++channel)
        ch32v103_write16(base + TIM_CH1CVR + 4u * channel, 0u);
    period_ticks[id] = config->period_ticks;
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

/* Each CHCTLR register holds two channels. The caller holds the IRQ lock. */
static void set_output_mode(uintptr_t base, hal_pwm_channel_t channel, uint16_t mode)
{
    uintptr_t control = base + (channel < HAL_PWM_CHANNEL_2 ? TIM_CHCTLR1 : TIM_CHCTLR2);
    unsigned shift = ((unsigned)channel & 1u) * 8u;
    uint16_t modes = ch32v103_read16(control);
    uint16_t updated = (modes & ~(TIM_OC_MODE << shift)) | (mode << shift);
    if (updated != modes)
        ch32v103_write16(control, updated);
}

static ecos_err_t set_high_time(hal_pwm_id_t id, hal_pwm_channel_t channel,
                                uint32_t value, bool is_percent)
{
    if (id >= PWM_INSTANCE_COUNT || (unsigned)channel >= HAL_PWM_CHANNEL_COUNT ||
        (id == 0u && channel != HAL_PWM_CHANNEL_0) || (is_percent && value > 100u))
        return ECOS_ERR_INVALID_ARGUMENT;

    uint32_t state = ch32v103_irq_lock();
    uint32_t period = period_ticks[id];
    if (period == 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    if (!is_percent && value > period) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_INVALID_ARGUMENT;
    }

    uint32_t ticks = is_percent ? period * value / 100u : value;
    uintptr_t base = timer_base[id];
    if (period == 1u) {
        /* ARR=0 stops counting. Neither constant level can wait for overflow. */
        set_output_mode(base, channel, ticks == 0u ? TIM_OC_INACTIVE : TIM_OC_ACTIVE);
    } else if (ticks > UINT16_MAX) {
        /* A full 65536-tick pulse cannot fit in the compare register. */
        set_output_mode(base, channel, TIM_OC_ACTIVE);
    } else {
        ch32v103_write16(base + TIM_CH1CVR + 4u * (unsigned)channel, (uint16_t)ticks);
        if (period == 65536u)
            set_output_mode(base, channel, TIM_OC_PWM1);
    }
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_pwm_set_pulse_ticks(hal_pwm_id_t id, hal_pwm_channel_t channel,
                                   uint32_t high_ticks)
{
    return set_high_time(id, channel, high_ticks, false);
}

ecos_err_t hal_pwm_set_duty_cycle(hal_pwm_id_t id, hal_pwm_channel_t channel,
                                  uint8_t duty_percent)
{
    return set_high_time(id, channel, duty_percent, true);
}

ecos_err_t hal_pwm_start(hal_pwm_id_t id)
{
    if (id >= PWM_INSTANCE_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;
    uint32_t state = ch32v103_irq_lock();
    if (period_ticks[id] == 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    uintptr_t base = timer_base[id];
    /* Only the first start after init needs to latch PSC, ARR and all CCRs.
     * Repeated starts and stop/resume must preserve the current frame. */
    if (ch32v103_read16(base + TIM_CCER) == 0u) {
        ch32v103_write16(base + TIM_SWEVGR, TIM_UG);
        ch32v103_write16(base + TIM_CCER, id == 0u ? TIM_CC1E : TIM_CC1E_TO_CC4E);
    }
    ch32v103_write16(base + TIM_CTLR1, TIM_ARPE | TIM_CEN);
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_pwm_stop(hal_pwm_id_t id)
{
    if (id >= PWM_INSTANCE_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;
    uint32_t state = ch32v103_irq_lock();
    if (period_ticks[id] == 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    ch32v103_write16(timer_base[id] + TIM_CTLR1, TIM_ARPE);
    ch32v103_irq_restore(state);
    return ECOS_OK;
}
