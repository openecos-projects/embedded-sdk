#include "ecos/bsp/console.h"
#include "ecos/device/sgp30.h"
#include "ecos/driver/i2c.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "i2c-sgp30"
#define SAMPLE_COUNT 20u
#define SAMPLE_INTERVAL_MS 1000u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    const ecos_i2c_config_t i2c_config = ECOS_I2C_CONFIG_DEFAULT;
    const ecos_sgp30_config_t sgp30_config = ECOS_SGP30_CONFIG_DEFAULT;
    ecos_sgp30_t sensor;
    uint64_t serial_id = 0u;
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
        ecos_sgp30_init(&sensor, &sgp30_config),
        "initialize SGP30"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_sgp30_read_serial_id(&sensor, &serial_id),
        "read SGP30 serial id"
    );
    (void)ECOS_LOGI(
        LOG_TAG,
        "SGP30 serial: 0x%08X%08X, sampling %u times",
        (unsigned)(serial_id >> 32),
        (unsigned)(serial_id & 0xFFFFFFFFu),
        SAMPLE_COUNT
    );
    (void)ECOS_LOGI(
        LOG_TAG,
        "note: IAQ readings are defaults for the first 15 s after init"
    );

    for (index = 0u; index < SAMPLE_COUNT; ++index) {
        ecos_sgp30_air_quality_t air_quality;

        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_sgp30_measure_air_quality(&sensor, &air_quality),
            "measure air quality"
        );
        (void)ECOS_LOGI(
            LOG_TAG,
            "[%u] CO2eq: %u ppm, TVOC: %u ppb",
            index,
            (unsigned)air_quality.co2_eq_ppm,
            (unsigned)air_quality.tvoc_ppb
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, SAMPLE_INTERVAL_MS),
            "wait for next sample"
        );
    }

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_sgp30_deinit(&sensor), "deinitialize SGP30"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_deinit(ECOS_I2C_DEFAULT),
        "deinitialize I2C controller"
    );
    (void)ECOS_LOGI(LOG_TAG, "done");
    halt();
}
