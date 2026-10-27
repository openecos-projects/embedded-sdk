#include "ecos/bsp/console.h"
#include "ecos/driver/backup.h"
#include "ecos/log.h"

#define LOG_TAG "backup-registers"

int main(void)
{
    uint16_t value;
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_backup_write(ECOS_BACKUP_DEFAULT, 0u, 0xA55Au),
                        "write backup register");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_backup_read(ECOS_BACKUP_DEFAULT, 0u, &value),
                        "read backup register");
    (void)ECOS_LOGI(LOG_TAG, "DR1: 0x%04X", (unsigned)value);
    for (;;) __asm__ volatile("nop");
}
