#include "hal_sys_uart.h"
#include "generated/autoconf.h"
#include "board.h"

/**
 * 初始化系统串口的波特率和数据格式。
 */
void hal_sys_uart_init(void)
{
    uint32_t baud_rate = (uint32_t)CONFIG_UART_BAUD_RATE;
    uint32_t divisor;

    /* 防止无效配置触发除零，并恢复默认波特率。 */
    if (baud_rate == 0u)
        baud_rate = 115200u;

    /* 根据 20 MHz APB 时钟计算最接近目标波特率的分频值。 */
    divisor = (STARTYSKY_T1_PICO_APB_CLOCK_HZ + (baud_rate * 8u)) /
              (baud_rate * 16u);
    if (divisor == 0u)
        divisor = 1u;

    /* 将过大的分频值限制到硬件十六位寄存器范围内。 */
    if (divisor > STARTYSKY_T1_PICO_UART_DIVISOR_MAX)
        divisor = STARTYSKY_T1_PICO_UART_DIVISOR_MAX;

    /* 使能分频锁存器并写入十六位分频值。 */
    REG_UART_0_LCR = STARTYSKY_T1_PICO_UART_LCR_DLAB;
    REG_UART_0_DLL = (uint8_t)divisor;
    REG_UART_0_DLH = (uint8_t)(divisor >> 8);

    /* 配置八位数据、一个停止位和无校验格式。 */
    REG_UART_0_LCR = 0x03u;
    REG_UART_0_MCR = 0x00u;
}


/**
 * 通过系统串口发送一个字符。
 */
void hal_sys_putchar(char c)
{
    /* 等待发送保持寄存器能够接收新字符。 */
    while ((REG_UART_0_LSR & STARTYSKY_T1_PICO_UART_LSR_TX_READY) == 0u)
    {
    }

    /* 将字符写入串口发送保持寄存器。 */
    REG_UART_0_THR = (uint8_t)c;
}


/**
 * 通过系统串口发送以空字符结尾的字符串。
 */
void hal_sys_putstr(char *str)
{
    /* 逐字符发送字符串，直到遇到结束标志。 */
    while (*str != '\0')
        hal_sys_putchar(*str++);
}


/**
 * 通过系统串口阻塞接收一个字符。
 */
uint8_t hal_sys_getchar(void)
{
    /* 等待接收保持寄存器中出现有效数据。 */
    while ((REG_UART_0_LSR & STARTYSKY_T1_PICO_UART_LSR_DATA_READY) == 0u)
    {
    }

    /* 返回串口接收保持寄存器中的字符。 */
    return REG_UART_0_RBR;
}
