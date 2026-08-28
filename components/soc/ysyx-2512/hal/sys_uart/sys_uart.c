#include "hal_sys_uart.h"
#include "ysyx_2512_soc.h"


/**
 * 初始化 L4 系统串口。
 */
void hal_sys_uart_init(void)
{
    /* 配置除数锁存并设置 8N1 帧格式。 */
    REG_UART_0_LC |= 0x80u;
    REG_UART_0_TH = 13u;
    REG_UART_0_IE = 0u;
    REG_UART_0_LC = 0x03u;
    REG_UART_0_IE = 0u;
}


/**
 * 向 L4 系统串口发送一个字符。
 */
void hal_sys_putchar(char c)
{
    /* 换行前发送回车，保持串口终端显示一致。 */
    if (c == '\n')
    {
        while ((REG_UART_0_LS & 0x20u) == 0u)
            ;
        REG_UART_0_TH = '\r';
    }

    /* 等待发送寄存器可用并写入字符。 */
    while ((REG_UART_0_LS & 0x20u) == 0u)
        ;
    REG_UART_0_TH = (uint8_t)c;
}


/**
 * 向 L4 系统串口发送字符串。
 */
void hal_sys_putstr(char *str)
{
    /* 按顺序发送字符串中的每个字符。 */
    while (*str != '\0')
        hal_sys_putchar(*str++);
}


/**
 * 阻塞读取一个 L4 系统串口字符。
 */
uint8_t hal_sys_getchar(void)
{
    /* 等待接收 FIFO 出现数据。 */
    while ((REG_UART_0_LS & 0x01u) == 0u)
        ;

    /* 返回接收寄存器中的字符。 */
    return REG_UART_0_RB;
}


/**
 * 尝试读取一个不阻塞的 UART 字符。
 */
uint8_t hal_sys_try_getchar(uint8_t *data)
{
    /* 检查接收 FIFO 是否有数据。 */
    if (data == 0 || (REG_UART_0_LS & 0x01u) == 0u)
        return 0u;

    /* 读取接收寄存器并报告成功。 */
    *data = REG_UART_0_RB;
    return 1u;
}
