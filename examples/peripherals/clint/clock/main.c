#include "ecos/bsp/console.h"
#include "ecos/driver/clint.h"
#include "ecos/log.h"

#define LOG_TAG "clint-clock"

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    ecos_clint_value_t value;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_clint_get_mtime(&value), "read mtime"
    );
    (void)ECOS_LOGI(
        LOG_TAG,
        "mtime: 0x%08X%08X (%u Hz)",
        value.high,
        value.low,
        ecos_clint_get_frequency_hz()
    );
    halt();
}
