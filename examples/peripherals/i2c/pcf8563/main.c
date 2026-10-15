#include "ecos/bsp/console.h"
#include "ecos/device/pcf8563.h"
#include "ecos/driver/i2c.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "i2c-pcf8563"
#define SAMPLE_COUNT 10u
#define SAMPLE_INTERVAL_MS 1000u

/* Reference time written to the RTC before reading it back. */
static const ecos_pcf8563_time_t initial_time = {
    0u,  /* second */
    30u, /* minute */
    12u, /* hour */
    17u, /* day */
    4u,  /* weekday */
    9u,  /* month */
    26u, /* year (2000 + 26) */
};

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

static void print_time(unsigned index, const ecos_pcf8563_time_t *time)
{
    (void)ECOS_LOGI(
        LOG_TAG,
        "[%u] 20%02u-%02u-%02u %02u:%02u:%02u (weekday %u)",
        index,
        (unsigned)time->year,
        (unsigned)time->month,
        (unsigned)time->day,
        (unsigned)time->hour,
        (unsigned)time->minute,
        (unsigned)time->second,
        (unsigned)time->weekday
    );
}

int main(void)
{
    const ecos_i2c_config_t i2c_config = ECOS_I2C_CONFIG_DEFAULT;
    const ecos_pcf8563_config_t rtc_config = ECOS_PCF8563_CONFIG_DEFAULT;
    ecos_pcf8563_t rtc;
    unsigned index;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_init(ECOS_I2C_DEFAULT, &i2c_config),
        "initialize I2C controller"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_pcf8563_init(&rtc, &rtc_config),
        "initialize PCF8563"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_pcf8563_set_time(&rtc, &initial_time),
        "write initial time"
    );
    (void)ECOS_LOGI(
        LOG_TAG, "initial time written, reading %u times", SAMPLE_COUNT
    );

    for (index = 0u; index < SAMPLE_COUNT; ++index) {
        ecos_pcf8563_time_t time;

        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ecos_pcf8563_get_time(&rtc, &time), "read time"
        );
        print_time(index, &time);
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, SAMPLE_INTERVAL_MS),
            "wait for next sample"
        );
    }

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_pcf8563_deinit(&rtc), "deinitialize PCF8563"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_deinit(ECOS_I2C_DEFAULT),
        "deinitialize I2C controller"
    );
    (void)ECOS_LOGI(LOG_TAG, "done");
    halt();
}
