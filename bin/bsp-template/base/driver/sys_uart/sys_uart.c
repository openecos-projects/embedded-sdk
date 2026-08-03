#include "hal_sys_uart.h"
#include "board.h"

/* @BSP_NAME@ 系统串口 HAL 实现。 */

/* [不要修改] 函数名称和参数由 SDK hal_sys_uart.h 定义。 */
void hal_sys_uart_init(void)
{
    /* [必须修改] TODO_BSP_REQUIRED：根据时钟和目标波特率配置串口。 */
}

void hal_sys_putchar(char c)
{
    /* [必须修改] TODO_BSP_REQUIRED：等待发送条件满足后写入真实数据寄存器。 */
    REG_UART_0_DATA = (uint32_t)(uint8_t)c;
}

void hal_sys_putstr(char *str)
{
    while (*str != '\0') {
        hal_sys_putchar(*str++);
    }
}

uint8_t hal_sys_getchar(void)
{
    /* [必须修改] TODO_BSP_REQUIRED：等待接收数据有效后返回一个字节。 */
    return (uint8_t)REG_UART_0_DATA;
}
