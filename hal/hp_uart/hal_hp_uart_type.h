#ifndef HAL_HP_UART_TYPE_H__
#define HAL_HP_UART_TYPE_H__

#include <stdint.h>
#include <stddef.h>

typedef enum{
    HP_UART_BAUDRATE_9600    = 9600,
    HP_UART_BAUDRATE_115200  = 115200,
} hp_uart_baudrate_t;

#define HP_UART_PARITY_NONE       0
#define HP_UART_ODD_PARITY        (0b00 << 8)
#define HP_UART_EVEN_PARITY       (0b01 << 8)

#define HP_UART_STOP_BITS_1       (0b00 << 6)
#define HP_UART_STOP_BITS_2       (0b01 << 6)

typedef struct{
    hp_uart_baudrate_t  baudrate;
    uint8_t             stop_bits;
    uint8_t             parity;
    uint8_t             data_bits;
} hp_uart_config_t;

typedef struct{
    uint32_t            instance;
    hp_uart_config_t    init;
    uint8_t             state;
    uint32_t error_code;
} hal_hp_uart_t;

#endif