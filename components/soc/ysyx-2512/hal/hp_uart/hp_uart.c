#include "hal_hp_uart.h"
#include "generated/autoconf.h"
#include "ysyx_2512_soc.h"

#define UART1_GPIO_MASK       ((uint32_t)0x3u << 25)
#define UART1_DEFAULT_LCR     0x1Fu

static uint32_t hp_uart_clk_hz(void)
{
#ifdef CONFIG_CPU_FREQ_MHZ
    return (uint32_t)CONFIG_CPU_FREQ_MHZ * 1000000u;
#else
    return 50000000u;
#endif
}

static void hp_uart_select_gpio(void)
{
    /*
     * L4: GPIO0[25] = UART1.TX, GPIO0[26] = UART1.RX, alternate function 0.
     */
    REG_GPIO_0_IOFCFG |= UART1_GPIO_MASK;
    REG_GPIO_0_PINMUX &= ~UART1_GPIO_MASK;
}

void hal_hp_uart_init(uint32_t baudrate){
    hp_uart_select_gpio();

    REG_UART_1_LCR = 0u;
    REG_UART_1_DIV = hp_uart_clk_hz() / baudrate - 1u;
    REG_UART_1_FCR = 0x0Fu;
    REG_UART_1_FCR = 0x0Cu;
    REG_UART_1_LCR = UART1_DEFAULT_LCR;
}

void hal_hp_uart_config(hp_uart_config_t *config){
    hp_uart_select_gpio();

    REG_UART_1_LCR = 0u;
    REG_UART_1_DIV = hp_uart_clk_hz() / config->baudrate - 1u;
    REG_UART_1_FCR = 0x0F;
    REG_UART_1_FCR = 0x0C;

    if (config->data_bits == 8u && config->parity == 0u && config->stop_bits == 0u) {
        REG_UART_1_LCR = UART1_DEFAULT_LCR;
    } else {
        REG_UART_1_LCR = (uint32_t)(config->data_bits - 5u) |
                         ((uint32_t)config->parity << 1) |
                         ((uint32_t)config->stop_bits << 2);
    }
}

void hal_hp_uart_send(char c){
    while(((REG_UART_1_LSR & 0x100) >> 8) == 1);
    REG_UART_1_TRX = (uint32_t)(uint8_t)c;
}


void hal_hp_uart_putstr(char *str){
    while (*str) {
        hal_hp_uart_send(*str++);
    }
}

void hal_hp_uart_recv(char *c){
    while(((REG_UART_1_LSR & 0x080) >> 7) == 1);
    *c = REG_UART_1_TRX;
}

void hal_hp_uart_recv_str(char *str){
    while(1){
        hal_hp_uart_recv(str++);
        if(*(str-1) == '\r' || *(str-1) == '\n'){
            *str = '\0';
            break;
        }
    }
}
