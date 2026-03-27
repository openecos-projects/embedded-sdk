#ifndef __HAL_SYS_UART_H__
#define __HAL_SYS_UART_H__

#include <stdint.h>

void hal_sys_uart_init(void);
void hal_sys_putchar(char c);
void hal_sys_putstr(char *str);

#endif