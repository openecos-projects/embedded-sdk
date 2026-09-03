#include "ecos/bsp/console.h"
#include "ecos/log.h"

int main(void)
{
    ecos_err_t result = bsp_console_init();

    if (ecos_result_succeeded(result))
        (void)ECOS_LOGI("hello", "Hello, World!");

    for (;;)
        __asm__ volatile("nop");
}
