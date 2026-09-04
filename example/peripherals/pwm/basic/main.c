#include "ecos/bsp/console.h"
#include "ecos/driver/pwm.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define PWM_DEMO_CONTROLLER ECOS_PWM_DEFAULT
#define PWM_DEMO_CHANNEL ECOS_PWM_CHANNEL_0
#define PWM_CLOCK_DIVIDER 50u
#define PWM_PERIOD_TICKS 1000u
#define PWM_STAGE_DELAY_MS 2000u
#define LOG_TAG "pwm-basic"

static void stop_pwm_on_error(int result, const char *operation)
{
    if (ecos_result_succeeded(result))
        return;

    (void)ecos_pwm_stop(PWM_DEMO_CONTROLLER);
    ECOS_PANIC_ON_ERROR(LOG_TAG, result, operation);
}

static void wait_for_next_stage(void)
{
    stop_pwm_on_error(
        ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, PWM_STAGE_DELAY_MS),
        "wait for next PWM stage"
    );
}

int main(void)
{
    const ecos_pwm_config_t config = {
        .clock_divider = PWM_CLOCK_DIVIDER,
        .period_ticks = PWM_PERIOD_TICKS,
    };
    int instance_count;
    int timer_count;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );

    instance_count = ecos_pwm_get_instance_count();
    ECOS_PANIC_ON_ERROR(LOG_TAG, instance_count, "query PWM controllers");
    if (instance_count <= (int)PWM_DEMO_CONTROLLER)
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ECOS_ERR_NOT_FOUND, "find default PWM controller"
        );
    timer_count = ecos_timer_get_instance_count();
    ECOS_PANIC_ON_ERROR(LOG_TAG, timer_count, "query timers");
    if (timer_count < 1)
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ECOS_ERR_NOT_FOUND, "find default timer"
        );

    stop_pwm_on_error(
        ecos_pwm_init(PWM_DEMO_CONTROLLER, &config),
        "configure PWM controller"
    );

    (void)ECOS_LOGI(
        LOG_TAG,
        "Configured PWM%u channel %u: divider=%u, period=%u ticks",
        (unsigned)PWM_DEMO_CONTROLLER,
        (unsigned)PWM_DEMO_CHANNEL,
        PWM_CLOCK_DIVIDER,
        PWM_PERIOD_TICKS
    );

    for (;;) {
        stop_pwm_on_error(
            ecos_pwm_set_duty_cycle(
                PWM_DEMO_CONTROLLER, PWM_DEMO_CHANNEL, 25u
            ),
            "set PWM duty cycle to 25 percent"
        );

        stop_pwm_on_error(
            ecos_pwm_start(PWM_DEMO_CONTROLLER), "start PWM controller"
        );
        (void)ECOS_LOGI(LOG_TAG, "PWM started: duty=25%%");
        wait_for_next_stage();

        stop_pwm_on_error(
            ecos_pwm_set_duty_cycle(
                PWM_DEMO_CONTROLLER, PWM_DEMO_CHANNEL, 75u
            ),
            "set PWM duty cycle to 75 percent"
        );
        (void)ECOS_LOGI(LOG_TAG, "PWM duty updated: duty=75%%");
        wait_for_next_stage();

        stop_pwm_on_error(
            ecos_pwm_stop(PWM_DEMO_CONTROLLER), "stop PWM controller"
        );
        (void)ECOS_LOGI(LOG_TAG, "PWM stopped");
        wait_for_next_stage();
    }
}
