/**
 * @file sys_uart_driver.c
 * @brief StarrySky C2 System UART driver implementation
 */

#include "sys_uart.h"
#include "board.h"  // Include C2 board header for register definitions

// Static function declarations
static status_t sys_uart_init_impl(sys_uart_port_t port);
static status_t sys_uart_putchar_impl(sys_uart_port_t port, char c);
static status_t sys_uart_puts_impl(sys_uart_port_t port, const char* str);

// System UART driver instance for StarrySky C2
static sys_uart_driver_t starrysky_c2_sys_uart_driver = {
    .base = {
        .version = DRIVER_VERSION(1, 0),
        .name = "starrysky_c2_sys_uart"
    },
    .init = sys_uart_init_impl,
    .putchar = sys_uart_putchar_impl,
    .puts = sys_uart_puts_impl
};

status_t starrysky_c2_sys_uart_init(void)
{
    return sys_uart_register_driver(&starrysky_c2_sys_uart_driver);
}

static status_t sys_uart_init_impl(sys_uart_port_t port)
{
    // C2 System UART uses fixed clock divider
    // Default baud rate is determined by the hardware
    // No additional initialization needed for basic functionality
    
    // For C2, we assume port 0 is the only system UART
    if (port != 0) {
        return STATUS_INVALID_ARG;
    }
    
    return STATUS_SUCCESS;
}

static status_t sys_uart_putchar_impl(sys_uart_port_t port, char c)
{
    // Validate port number
    if (port != 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Wait for UART to be ready (simple busy wait)
    // C2 System UART doesn't have status register, so we assume it's ready
    
    // Write character to UART data register
    REG_UART_0_DATA = (uint32_t)c;
    
    return STATUS_SUCCESS;
}

static status_t sys_uart_puts_impl(sys_uart_port_t port, const char* str)
{
    CHECK_NULL(str);
    
    // Validate port number
    if (port != 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Send each character in the string
    while (*str != '\0') {
        REG_UART_0_DATA = (uint32_t)(*str);
        str++;
    }
    
    return STATUS_SUCCESS;
}