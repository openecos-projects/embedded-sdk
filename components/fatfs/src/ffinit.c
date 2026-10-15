#include "ffinit.h"

#include "ecos/log.h"

FATFS fs_Flash;
FRESULT res_Flash;
static uint8_t work_buf[FF_MIN_SS] __attribute__((aligned(4)));

bool load_filesystem(void)
{
    res_Flash = f_mount(&fs_Flash, "0:", 1);
    if (res_Flash == FR_OK) {
        (void)ECOS_LOGI("fatfs", "File system mounted");
        return true;
    }

    (void)ECOS_LOGE("fatfs", "File system mount failed: %d", (int)res_Flash);
    return false;
}

FRESULT format_filesystem(void)
{
    static const MKFS_PARM options = {
        .fmt = FM_FAT,
        .n_fat = 2,
        .align = 0,
        .n_root = 0,
        .au_size = FF_MIN_SS,
    };

    (void)f_mount(NULL, "0:", 0);
    res_Flash = f_mkfs("0:", &options, work_buf, sizeof(work_buf));
    if (res_Flash != FR_OK) {
        (void)ECOS_LOGE("fatfs", "File system format failed: %d", (int)res_Flash);
        return res_Flash;
    }

    if (!load_filesystem()) {
        return res_Flash;
    }
    (void)ECOS_LOGI("fatfs", "File system formatted and mounted");
    return FR_OK;
}
