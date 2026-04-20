#include "hal_hp_uart.h"
#include "generated/autoconf.h"
#include "board.h"


void hal_hp_uart_init(uint32_t baudrate){
#if CONFIG_HP_UART_IP_ID == 0
    REG_UART_1_LCR = 0x00;
    REG_UART_1_DIV = (CONFIG_CPU_FREQ_MHZ * 1000000 / baudrate) - 1;
    REG_UART_1_FCR = 0x0F;
    REG_UART_1_FCR = 0x0C;
    REG_UART_1_LCR = 0x1F;
#elif CONFIG_HP_UART_IP_ID == 1
#endif
}

void hal_hp_uart_config(hp_uart_config_t *config){
    REG_UART_1_LCR = 0x00;
    REG_UART_1_DIV = (CONFIG_CPU_FREQ_MHZ * 1000000 / config->baudrate) - 1;
    REG_UART_1_FCR = 0x0F;
    REG_UART_1_FCR = 0x0C;

    REG_UART_1_LCR = (config->data_bits - 5) << 0 |
                     (config->parity << 1) |
                     (config->stop_bits << 2);
}

void hal_hp_uart_send(char c){
#if CONFIG_HP_UART_IP_ID == 0
    while(((REG_UART_1_LSR & 0x100) >> 8) == 1);
    REG_UART_1_TRX = c;
#elif CONFIG_HP_UART_IP_ID == 1
#endif
}


void hal_hp_uart_putstr(char *str){
#if CONFIG_HP_UART_IP_ID == 0
    while (*str) {
        hal_hp_uart_send(*str++);
    }
#elif CONFIG_HP_UART_IP_ID == 1
#endif
}

void hal_hp_uart_recv(char *c){
#if CONFIG_HP_UART_IP_ID == 0
    while(((REG_UART_1_LSR & 0x080) >> 7) == 1);
    *c = REG_UART_1_TRX;
#elif CONFIG_HP_UART_IP_ID == 1
#endif
}

void hal_hp_uart_recv_str(char *str){
#if CONFIG_HP_UART_IP_ID == 0
    while(1){
        hal_hp_uart_recv(str++);
        if(*(str-1) == '\n'){
            *str = 0;
            break;
        }
    }
#elif CONFIG_HP_UART_IP_ID == 1
#endif
}