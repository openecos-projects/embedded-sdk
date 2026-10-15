#include "ecos/bsp/console.h"
#include "ecos/driver/ps2.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "ps2-keycodes"
#define PS2_ESCAPE_SCAN_CODE 0x76u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    uint32_t scan_code;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_ps2_init(ECOS_PS2_DEFAULT), "initialize PS2");
    (void)ECOS_LOGI(LOG_TAG, "press Escape to stop");

    do {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ecos_ps2_read(ECOS_PS2_DEFAULT, &scan_code), "read scan code"
        );
        if (scan_code != 0u)
            (void)ECOS_LOGI(LOG_TAG, "scan code: 0x%08X", scan_code);
    } while (scan_code != PS2_ESCAPE_SCAN_CODE);

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_ps2_deinit(ECOS_PS2_DEFAULT), "deinitialize PS2"
    );
    halt();
}
