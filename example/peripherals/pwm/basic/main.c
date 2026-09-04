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

static void stop_with_error(ecos_err_t error, const char *operation)
{
    (void)ecos_pwm_stop(PWM_DEMO_CONTROLLER);
    (void)ECOS_LOG_ERR(LOG_TAG, error, operation);
    for (;;)
        __asm__ volatile("nop");
}

static void wait_for_next_stage(void)
{
    ecos_err_t result = ecos_timer_delay_ms(
        ECOS_TIMER_DEFAULT, PWM_STAGE_DELAY_MS
    );

    if (ecos_result_failed(result))
        stop_with_error(result, "wait for next PWM stage");
}

int main(void)
{
    const ecos_pwm_config_t config = {
        .clock_divider = PWM_CLOCK_DIVIDER,
        .period_ticks = PWM_PERIOD_TICKS,
    };
    ecos_err_t result;
    int instance_count;
    int timer_count;

    result = bsp_console_init();
    if (ecos_result_failed(result))
        stop_with_error(result, "initialize console");

    instance_count = ecos_pwm_get_instance_count();
    if (instance_count < 0)
        stop_with_error((ecos_err_t)instance_count, "query PWM controllers");
    if (instance_count <= (int)PWM_DEMO_CONTROLLER)
        stop_with_error(ECOS_ERR_NOT_FOUND, "find default PWM controller");
    timer_count = ecos_timer_get_instance_count();
    if (timer_count < 0)
        stop_with_error((ecos_err_t)timer_count, "query timers");
    if (timer_count < 1)
        stop_with_error(ECOS_ERR_NOT_FOUND, "find default timer");

    result = ecos_pwm_init(PWM_DEMO_CONTROLLER, &config);
    if (ecos_result_failed(result))
        stop_with_error(result, "configure PWM controller");

    (void)ECOS_LOGI(
        LOG_TAG,
        "Configured PWM%u channel %u: divider=%u, period=%u ticks",
        (unsigned)PWM_DEMO_CONTROLLER,
        (unsigned)PWM_DEMO_CHANNEL,
        PWM_CLOCK_DIVIDER,
        PWM_PERIOD_TICKS
    );

    for (;;) {
        result = ecos_pwm_set_duty_cycle(
            PWM_DEMO_CONTROLLER, PWM_DEMO_CHANNEL, 25u
        );
        if (ecos_result_failed(result))
            stop_with_error(result, "set PWM duty cycle to 25 percent");

        result = ecos_pwm_start(PWM_DEMO_CONTROLLER);
        if (ecos_result_failed(result))
            stop_with_error(result, "start PWM controller");
        (void)ECOS_LOGI(LOG_TAG, "PWM started: duty=25%%");
        wait_for_next_stage();

        result = ecos_pwm_set_duty_cycle(
            PWM_DEMO_CONTROLLER, PWM_DEMO_CHANNEL, 75u
        );
        if (ecos_result_failed(result))
            stop_with_error(result, "set PWM duty cycle to 75 percent");
        (void)ECOS_LOGI(LOG_TAG, "PWM duty updated: duty=75%%");
        wait_for_next_stage();

        result = ecos_pwm_stop(PWM_DEMO_CONTROLLER);
        if (ecos_result_failed(result))
            stop_with_error(result, "stop PWM controller");
        (void)ECOS_LOGI(LOG_TAG, "PWM stopped");
        wait_for_next_stage();
    }
}
