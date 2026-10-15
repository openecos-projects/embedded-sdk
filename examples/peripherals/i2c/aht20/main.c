#include "ecos/bsp/console.h"
#include "ecos/device/aht20.h"
#include "ecos/driver/i2c.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "i2c-aht20"
#define SAMPLE_COUNT 10u
#define SAMPLE_INTERVAL_MS 1000u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

/* The rv32e runtime links without libgcc division helpers, so split the
 * x100 fixed-point values by repeated subtraction instead of / and %. */
static void split_x100(uint32_t value, uint32_t *whole, uint32_t *fraction)
{
    uint32_t high = 0u;

    while (value >= 100u) {
        value -= 100u;
        ++high;
    }
    *whole = high;
    *fraction = value;
}

static void print_sample(unsigned index, const ecos_aht20_data_t *data)
{
    int32_t temperature = data->temperature_x100;
    const char *sign = "";
    uint32_t magnitude;
    uint32_t whole;
    uint32_t fraction;

    if (temperature < 0) {
        sign = "-";
        magnitude = (uint32_t)(-temperature);
    } else {
        magnitude = (uint32_t)temperature;
    }
    split_x100(magnitude, &whole, &fraction);
    (void)ECOS_LOGI(
        LOG_TAG, "[%u] temperature: %s%u.%02u C", index, sign, whole, fraction
    );
    split_x100(data->humidity_x100, &whole, &fraction);
    (void)ECOS_LOGI(LOG_TAG, "[%u] humidity: %u.%02u %%", index, whole, fraction);
}

int main(void)
{
    const ecos_i2c_config_t i2c_config = ECOS_I2C_CONFIG_DEFAULT;
    const ecos_aht20_config_t aht20_config = ECOS_AHT20_CONFIG_DEFAULT;
    ecos_aht20_t sensor;
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
        ecos_aht20_init(&sensor, &aht20_config),
        "initialize AHT20"
    );

    (void)ECOS_LOGI(LOG_TAG, "AHT20 ready, sampling %u times", SAMPLE_COUNT);

    for (index = 0u; index < SAMPLE_COUNT; ++index) {
        ecos_aht20_data_t data;

        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ecos_aht20_read(&sensor, &data), "read AHT20"
        );
        print_sample(index, &data);
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, SAMPLE_INTERVAL_MS),
            "wait for next sample"
        );
    }

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_aht20_deinit(&sensor), "deinitialize AHT20"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_deinit(ECOS_I2C_DEFAULT),
        "deinitialize I2C controller"
    );
    (void)ECOS_LOGI(LOG_TAG, "done");
    halt();
}
