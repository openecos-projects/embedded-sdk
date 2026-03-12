/**
 * @file hp_uart_driver.c
 * @brief StarrySky C2 HP UART driver implementation
 */

#include "hp_uart.h"
#include "board.h"  // Include C2 board header for register definitions

// Static function declarations
static status_t hp_uart_init_impl(uint32_t baud_rate);
static status_t hp_uart_config_impl(const hp_uart_config_t* config);
static status_t hp_uart_send_impl(char c);
static status_t hp_uart_send_str_impl(const char* str);
static status_t hp_uart_receive_impl(char* c);
static status_t hp_uart_receive_str_impl(char* str, size_t max_len);

// HP UART driver instance for StarrySky C2
static hp_uart_driver_t starrysky_c2_hp_uart_driver = {
    .base = {
        .version = DRIVER_VERSION(1, 0),
        .name = "starrysky_c2_hp_uart"
    },
    .init = hp_uart_init_impl,
    .config = hp_uart_config_impl,
    .send = hp_uart_send_impl,
    .send_str = hp_uart_send_str_impl,
    .receive = hp_uart_receive_impl,
    .receive_str = hp_uart_receive_str_impl
};

status_t starrysky_c2_hp_uart_init(void)
{
    return hp_uart_register_driver(&starrysky_c2_hp_uart_driver);
}

// Helper function to calculate baud rate divisor
static uint32_t calculate_baud_divisor(uint32_t baud_rate)
{
    // Assuming system clock is known, calculate divisor
    // For simplicity, using direct mapping for common baud rates
    switch (baud_rate) {
        case 9600:
            return 10416;   // Example divisor for 100MHz clock
        case 19200:
            return 5208;
        case 38400:
            return 2604;
        case 57600:
            return 1736;
        case 115200:
            return 868;
        default:
            return 868; // Default to 115200
    }
}

static status_t hp_uart_init_impl(uint32_t baud_rate)
{
    // Calculate and set baud rate divisor
    uint32_t divisor = calculate_baud_divisor(baud_rate);
    REG_UART_1_DIV = divisor;
    
    // Set line control register: 8N1 (8 data bits, no parity, 1 stop bit)
    REG_UART_1_LCR = (0x03 << 0); // 8 data bits
    
    return STATUS_SUCCESS;
}

static status_t hp_uart_config_impl(const hp_uart_config_t* config)
{
    CHECK_NULL(config);
    
    // Set baud rate
    uint32_t divisor = calculate_baud_divisor(config->baud_rate);
    REG_UART_1_DIV = divisor;
    
    // Configure line control register
    uint32_t lcr = 0;
    
    // Data bits (5-8)
    if (config->data_bits >= 5 && config->data_bits <= 8) {
        lcr |= ((config->data_bits - 5) & 0x03) << 0;
    }
    
    // Stop bits (1 or 2)
    if (config->stop_bits == HP_UART_STOP_BITS_2) {
        lcr |= (1 << 2);
    }
    
    // Parity
    if (config->parity == HP_UART_ODD_PARITY) {
        lcr |= (0x01 << 3) | (0x01 << 4); // Enable parity + odd
    } else if (config->parity == HP_UART_EVEN_PARITY) {
        lcr |= (0x01 << 3) | (0x00 << 4); // Enable parity + even
    } else if (config->parity == HP_UART_ZERO_PARITY) {
        lcr |= (0x01 << 3) | (0x02 << 4); // Enable parity + zero
    } else if (config->parity == HP_UART_ONE_PARITY) {
        lcr |= (0x01 << 3) | (0x03 << 4); // Enable parity + one
    }
    // HP_UART_PARITY_NONE: parity disabled (default)
    
    REG_UART_1_LCR = lcr;
    
    return STATUS_SUCCESS;
}

static status_t hp_uart_send_impl(char c)
{
    // Wait for transmit FIFO not full
    while ((REG_UART_1_LSR & (1 << 5)) == 0); // Check THRE bit
    
    // Send character
    REG_UART_1_TRX = (uint32_t)c;
    
    return STATUS_SUCCESS;
}

static status_t hp_uart_send_str_impl(const char* str)
{
    CHECK_NULL(str);
    
    while (*str != '\0') {
        hp_uart_send_impl(*str);
        str++;
    }
    
    return STATUS_SUCCESS;
}

static status_t hp_uart_receive_impl(char* c)
{
    CHECK_NULL(c);
    
    // Wait for receive data available
    while ((REG_UART_1_LSR & (1 << 0)) == 0); // Check DR bit
    
    // Read character
    *c = (char)(REG_UART_1_TRX & 0xFF);
    
    return STATUS_SUCCESS;
}

static status_t hp_uart_receive_str_impl(char* str, size_t max_len)
{
    CHECK_NULL(str);
    
    if (max_len == 0) {
        return STATUS_INVALID_ARG;
    }
    
    size_t i = 0;
    char c;
    
    while (i < (max_len - 1)) {
        hp_uart_receive_impl(&c);
        if (c == '\n' || c == '\r') {
            break;
        }
        str[i++] = c;
    }
    
    str[i] = '\0';
    
    return STATUS_SUCCESS;
}