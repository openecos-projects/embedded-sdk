#ifndef __HP_UART_TYPE_H__
#define __HP_UART_TYPE_H__

#include <stdint.h>

typedef enum{
    HP_UART_BAUDRATE_9600 = 9600,
    HP_UART_BAUDRATE_19200 = 19200,
    HP_UART_BAUDRATE_38400 = 38400,
    HP_UART_BAUDRATE_57600 = 57600,
    HP_UART_BAUDRATE_115200 = 115200,
}hp_uart_baudrate_t;

#define HP_UART_PARITY_NONE 0
#define HP_UART_ODD_PARITY 0b00 << 8
#define HP_UART_EVEN_PARITY 0b01 << 8
#define HP_UART_ZERO_PARITY 0b10 << 8
#define HP_UART_ONE_PARITY 0b11 << 8
#define HP_UART_STOP_BITS_1 0b00 << 6
#define HP_UART_STOP_BITS_2 0b01 << 6

typedef struct {
    hp_uart_baudrate_t baudrate;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t data_bits;
}hp_uart_config_t;

#endif