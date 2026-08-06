#include "main.h"

/**
 * 停止测试程序并保留当前现场。
 */
__attribute__((noreturn)) static void timer_test_halt(void)
{
    /* 使处理器保持稳定循环，便于观察最后一条串口输出。 */
    for (;;)
        __asm__ volatile("nop");
}


/**
 * 初始化系统串口并持续验证 Timer0 一秒轮询延时。
 */
int main(void)
{
    uint8_t status;

    /* 初始化串口并输出 Timer0 测试配置。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1 Timer0 polling test started.\n");
    hal_sys_putstr("Expected interval: 1 second.\n");

    /* 持续执行一秒硬件延时并输出本轮结果。 */
    for (;;)
    {
        status = hal_delay_s(0u, 1u);
        if (status != 0u)
        {
            hal_sys_putstr("Timer0 FAIL: completion status timeout.\n");
            timer_test_halt();
        }

        hal_sys_putstr("Timer0 1-second delay PASS.\n");
    }
}
