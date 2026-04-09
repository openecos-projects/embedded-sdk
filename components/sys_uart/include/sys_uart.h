#ifndef __SYS_UART_H__
#define __SYS_UART_H__

#include <stdint.h>

void sys_uart_init(void);
void sys_putchar(char c);
void sys_putstr(char *str);

#endif