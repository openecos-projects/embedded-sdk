#include "ecos/log.h"
#include "sfud.h"

int main(void)
{
    sfud_flash *flash;
    sfud_err result;

    result = sfud_init();
    if (result != SFUD_SUCCESS) {
        (void)ECOS_LOGE("flash-sfud", "SFUD initialization failed: %d", (int)result);
        return 1;
    }

    flash = sfud_get_device(0u);
    if (flash == NULL || !flash->init_ok) {
        (void)ECOS_LOGE("flash-sfud", "No initialized serial flash device");
        return 1;
    }

    (void)ECOS_LOGI(
        "flash-sfud",
        "Flash %s ready, capacity %lu bytes",
        flash->chip.name,
        (unsigned long)flash->chip.capacity
    );
    return 0;
}
