#include "ecos/bsp/console.h"
#include "ecos/driver/rtc.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "rtc-counter"

/* 输入时钟由 SoC 决定；按 50 MHz 输入估算，50000 分频约为 1 kHz 计数。 */
#define RTC_COUNTER_PRESCALER 50000u
#define RTC_ALARM_DELTA_TICKS 1000u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    const ecos_rtc_config_t config = { RTC_COUNTER_PRESCALER, 1u };
    uint32_t counter_start;
    uint32_t counter_now;
    int alarm;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_rtc_init(ECOS_RTC_DEFAULT, &config), "initialize RTC"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_rtc_set_counter(ECOS_RTC_DEFAULT, 0u), "reset counter"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_rtc_get_counter(ECOS_RTC_DEFAULT, &counter_start),
        "read counter"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_rtc_set_alarm(
            ECOS_RTC_DEFAULT, counter_start + RTC_ALARM_DELTA_TICKS
        ),
        "set alarm"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 500u),
        "delay before re-reading"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_rtc_get_counter(ECOS_RTC_DEFAULT, &counter_now),
        "re-read counter"
    );
    (void)ECOS_LOGI(
        LOG_TAG, "counter: %u -> %u", counter_start, counter_now
    );

    do {
        alarm = ecos_rtc_alarm_triggered(ECOS_RTC_DEFAULT);
        ECOS_PANIC_ON_ERROR(LOG_TAG, alarm, "poll alarm status");
    } while (alarm == 0);
    (void)ECOS_LOGI(LOG_TAG, "alarm triggered");

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_rtc_deinit(ECOS_RTC_DEFAULT), "deinitialize RTC"
    );
    halt();
}
