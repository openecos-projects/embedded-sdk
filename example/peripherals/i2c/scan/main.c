#include "ecos/bsp/console.h"
#include "ecos/driver/i2c.h"
#include "ecos/log.h"

#define I2C_SCAN_CONTROLLER ECOS_I2C_DEFAULT
#define I2C_SCAN_FIRST_ADDRESS 0x08u
#define I2C_SCAN_LAST_ADDRESS 0x77u
#define LOG_TAG "i2c-scan"

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    const ecos_i2c_config_t config = ECOS_I2C_CONFIG_DEFAULT;
    unsigned address;
    unsigned device_count = 0u;
    int instance_count;
    int probe_result;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );

    instance_count = ecos_i2c_get_instance_count();
    ECOS_PANIC_ON_ERROR(LOG_TAG, instance_count, "query I2C controllers");
    if (instance_count <= (int)I2C_SCAN_CONTROLLER)
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ECOS_ERR_NOT_FOUND, "find default I2C controller"
        );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_init(I2C_SCAN_CONTROLLER, &config),
        "initialize I2C controller"
    );

    (void)ECOS_LOGI(
        LOG_TAG,
        "Scanning 7-bit I2C addresses 0x%02X-0x%02X",
        I2C_SCAN_FIRST_ADDRESS,
        I2C_SCAN_LAST_ADDRESS
    );

    for (address = I2C_SCAN_FIRST_ADDRESS;
         address <= I2C_SCAN_LAST_ADDRESS; ++address) {
        probe_result = ecos_i2c_probe(
            I2C_SCAN_CONTROLLER, (uint8_t)address
        );
        if (ecos_result_failed(probe_result)) {
            (void)ecos_i2c_deinit(I2C_SCAN_CONTROLLER);
            ECOS_PANIC_ON_ERROR(
                LOG_TAG, probe_result, "probe I2C address"
            );
        }
        if (probe_result == 1) {
            ++device_count;
            (void)ECOS_LOGI(LOG_TAG, "Device found at 0x%02X", address);
        }
    }

    (void)ECOS_LOGI(
        LOG_TAG, "Scan complete: %u device(s) found", device_count
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_deinit(I2C_SCAN_CONTROLLER),
        "deinitialize I2C controller"
    );
    halt();
}
