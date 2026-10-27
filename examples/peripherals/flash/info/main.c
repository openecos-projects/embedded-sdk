#include "ecos/bsp/console.h"
#include "ecos/driver/flash.h"
#include "ecos/log.h"

#define LOG_TAG "flash-info"

int main(void)
{
    uint32_t size;
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_flash_get_size(ECOS_FLASH_DEFAULT, &size),
                        "read flash size");
    (void)ECOS_LOGI(LOG_TAG, "on-chip flash: %u bytes, page %u bytes",
                    (unsigned)size, (unsigned)ECOS_FLASH_PAGE_SIZE);
    for (;;) __asm__ volatile("nop");
}
