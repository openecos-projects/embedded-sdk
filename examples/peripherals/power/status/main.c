#include "ecos/bsp/console.h"
#include "ecos/driver/power.h"
#include "ecos/log.h"

#define LOG_TAG "power-status"

int main(void)
{
    uint32_t flags;
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_power_get_flags(ECOS_POWER_DEFAULT, &flags),
                        "read power flags");
    (void)ECOS_LOGI(LOG_TAG, "power flags: 0x%08X", flags);
    for (;;) __asm__ volatile("nop");
}
