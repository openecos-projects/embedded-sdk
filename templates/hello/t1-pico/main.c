#include "main.h"

/**
 * 初始化系统串口、输出问候信息并保持程序运行。
 */
int main(void)
{
    /* 初始化系统串口并输出板卡识别信息。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1-Pico UART hello world.\n");

    /* 保持处理器运行，等待后续外设功能扩展。 */
    for (;;)
    {
        __asm__ volatile("nop");
    }
}
