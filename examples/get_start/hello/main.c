#include "ecos/bsp/console.h"
#include "ecos/log.h"

#define LOG_TAG "hello"

int main(void)
{
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );
    (void)ECOS_LOGI(LOG_TAG, "Hello, World!");

    for (;;)
        __asm__ volatile("nop");
}
