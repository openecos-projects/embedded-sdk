#include "hp_uart.h"
#include "generated/autoconf.h"
#include "stdio.h"
#include "board.h"

void hp_uart_init(uint32_t baudrate){
#if CONFIG_HP_UART_IP_ID == 0
    // 禁用UART
    REG_UART_1_LCR = 0x00;

    // 设置波特率
    REG_UART_1_DIV = (CONFIG_CPU_FREQ_MHZ * 1000000 / baudrate) - 1;

    // 启用FIFO和接收中断
    REG_UART_1_FCR = 0x0F;
    REG_UART_1_FCR = 0x0C;

    // 设置数据位为8位，无校验位，1位停止位
    REG_UART_1_LCR = 0x1F;
#elif CONFIG_HP_UART_IP_ID == 1

#endif
}

void hp_uart_send(char c){
#if CONFIG_HP_UART_IP_ID == 0
    while (((REG_UART_1_LSR & 0x100) >> 8) == 1);
    REG_UART_1_TRX = c;
#elif CONFIG_HP_UART_IP_ID == 1

#endif
}

void hp_uart_send_str(char* str){
#if CONFIG_HP_UART_IP_ID == 0
    while (*str)
    {
        hp_uart_send(*str++);
    }
#elif CONFIG_HP_UART_IP_ID == 1

#endif
}

void hp_uart_recv(char* c){
#if CONFIG_HP_UART_IP_ID == 0
    while (((REG_UART_1_LSR & 0x080) >> 7) == 1);
    *c = REG_UART_1_TRX;
    hp_uart_send(*c);
#elif CONFIG_HP_UART_IP_ID == 1

#endif
}

void hp_uart_recv_str(char* str){
#if CONFIG_HP_UART_IP_ID == 0
    while (1)
    {
        hp_uart_recv(str++);
        if (*(str - 1) == '\n')
        {
            break;
        }
    }
#elif CONFIG_HP_UART_IP_ID == 1

#endif
}
