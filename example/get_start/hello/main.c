#include "main.h"

int main(void)
{
    hal_sys_uart_init();
    hal_sys_putstr("Hello, World!\n\r");

    for (;;)
    {
        __asm__ volatile("nop");
    }
}
