#include "ecos/bsp/console.h"
#include "ecos/driver/wwdg.h"
#include "ecos/log.h"

#define LOG_TAG "wwdg-reset"

int main(void)
{
    const ecos_wwdg_config_t config = ECOS_WWDG_CONFIG_DEFAULT;
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_wwdg_init(ECOS_WWDG_DEFAULT, &config),
                        "initialize WWDG");
    (void)ECOS_LOGI(LOG_TAG, "WWDG enabled; intentionally stop feeding for reset test");
    for (;;) __asm__ volatile("nop");
}
