#include "ecos/bsp/console.h"
#include "ecos/driver/wdg.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "wdg-reset"

int main(void)
{
    const ecos_wdg_config_t config = ECOS_WDG_CONFIG_DEFAULT;
    uint32_t status;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    (void)ECOS_LOGI(LOG_TAG, "arming watchdog with reset control 0x%X", config.control);
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_wdg_init(ECOS_WDG_DEFAULT, &config), "initialize watchdog"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_wdg_feed(ECOS_WDG_DEFAULT), "feed watchdog"
    );

    for (;;) {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ecos_wdg_get_status(ECOS_WDG_DEFAULT, &status),
            "read watchdog status"
        );
        if (status != 0u) {
            (void)ECOS_LOGI(LOG_TAG, "watchdog status: 0x%08X", status);
            for (;;)
                __asm__ volatile("nop");
        }
    }
}
