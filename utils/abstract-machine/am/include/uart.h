#ifndef __UART_H
#define __UART_H

#include <stdio.h>
#include <am.h>
#include <klib-macros.h>

// #define YSYX2

#define UART_REG_RB *((volatile uint8_t *)0x10000000)
#define UART_REG_TH *((volatile uint8_t *)0x10000000)
#define UART_REG_IE *((volatile uint8_t *)0x10000001)
#define UART_REG_II *((volatile uint8_t *)0x10000002)
#define UART_REG_FC *((volatile uint8_t *)0x10000002)
#ifdef YSYX2
#define UART_REG_LC *((volatile uint8_t *)0x1000000F)
#else
#define UART_REG_LC *((volatile uint8_t *)0x10000003)
#endif
#define UART_REG_MC *((volatile uint8_t *)0x10000004)
#ifdef YSYX2
#define UART_REG_LS *((volatile uint8_t *)0x10000015)
#else
#define UART_REG_LS *((volatile uint8_t *)0x10000005)
#endif
#define UART_REG_MS *((volatile uint8_t *)0x10000006)

// void virt_uart_init(int baud, int freq);
void virt_uart_init();
void drv_uart_putc(char ch);
// int drv_uart_getc();

#endif /* end __UART_H */