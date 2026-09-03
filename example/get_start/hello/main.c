#include "ecos/bsp/console.h"

int main(void)
{
    static const char message[] = "Hello, World!\n";

    if (bsp_console_init() == 0)
        (void)bsp_console_write(message, sizeof(message) - 1u);

    for (;;)
        __asm__ volatile("nop");
}
