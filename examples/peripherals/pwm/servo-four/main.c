#include "ecos/bsp/console.h"
#include "ecos/driver/pwm.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define LOG_TAG "pwm-servo-four"
#define TIM3_PWM ((ecos_pwm_id_t)1u)

int main(void)
{
    const ecos_pwm_config_t config = {.clock_divider = 8u, .period_ticks = 20000u};
    const uint32_t initial_ticks[] = {1000u, 1490u, 1500u, 2000u};

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_pwm_init(TIM3_PWM, &config), "configure TIM3");
    for (unsigned i = 0; i < ECOS_PWM_CHANNEL_COUNT; ++i) {
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_pwm_set_pulse_ticks(TIM3_PWM, (ecos_pwm_channel_t)i,
                                                     initial_ticks[i]),
                            "set initial pulse width");
    }
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_pwm_start(TIM3_PWM), "start all four outputs");

    uint32_t next_ticks = 1510u;
    for (;;) {
        ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 2000u),
                            "wait before updating PB5");
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_pwm_set_pulse_ticks(TIM3_PWM, ECOS_PWM_CHANNEL_1, next_ticks),
                            "update PB5");
        (void)ECOS_LOGI(LOG_TAG, "PB5 pulse set to %u ticks", (unsigned)next_ticks);
        next_ticks = next_ticks == 1510u ? 1490u : 1510u;
    }
}
