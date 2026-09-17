#include "ecos/bsp/console.h"
#include "ecos/driver/archinfo.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "archinfo-chip-id"

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    uint32_t system_id;
    uint64_t chip_id;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_archinfo_get_system_id(ECOS_ARCHINFO_DEFAULT, &system_id),
        "read system id"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_archinfo_get_chip_id(ECOS_ARCHINFO_DEFAULT, &chip_id),
        "read chip id"
    );

    (void)ECOS_LOGI(LOG_TAG, "system id: 0x%08X", system_id);
    /* 按两个 32 位半字打印，避免 64 位格式化。 */
    (void)ECOS_LOGI(
        LOG_TAG,
        "chip id: 0x%08X%08X",
        (uint32_t)(chip_id >> 32),
        (uint32_t)chip_id
    );
    halt();
}
