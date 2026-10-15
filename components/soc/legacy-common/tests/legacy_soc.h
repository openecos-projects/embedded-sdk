#ifndef ECOS_TEST_LEGACY_SOC_H
#define ECOS_TEST_LEGACY_SOC_H

#include <stdint.h>

#define LEGACY_UART0_SIMPLE 1
#define LEGACY_HAS_UART1 0
#define REG_UART_0_CLKDIV test_uart0_clkdiv
#define REG_UART_0_DATA test_uart0_data

extern volatile uint32_t test_uart0_clkdiv;
extern volatile uint32_t test_uart0_data;

#endif
