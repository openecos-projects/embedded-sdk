#include "ecos/bsp/console.h"
#include "ecos/driver/crc.h"
#include "ecos/log.h"

#include <stddef.h>
#include <stdint.h>

#define LOG_TAG "crc-compute"

static const uint32_t g_data[] = {
    0x00123456u, 0x00ABCDEFu, 0x00000000u, 0xFFFFFFFFu,
};

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    const ecos_crc_config_t config = ECOS_CRC_CONFIG_DEFAULT;
    uint32_t result;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_crc_init(ECOS_CRC_DEFAULT, &config), "initialize CRC"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_crc_compute(
            ECOS_CRC_DEFAULT, g_data, sizeof(g_data) / sizeof(g_data[0]), &result
        ),
        "compute CRC"
    );
    (void)ECOS_LOGI(LOG_TAG, "CRC of %u word(s): 0x%08X",
                    (unsigned)(sizeof(g_data) / sizeof(g_data[0])), result);

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_crc_deinit(ECOS_CRC_DEFAULT), "deinitialize CRC"
    );
    halt();
}
