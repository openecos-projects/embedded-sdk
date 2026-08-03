#ifndef USER_BSP_BOARD_H
#define USER_BSP_BOARD_H

#include <stdint.h>

/* @BSP_NAME@ 寄存器和板级定义。 */
/* [必须修改] TODO_BSP_REQUIRED：根据芯片手册填写系统串口寄存器。 */
#define REG_UART_0_DATA   (*((volatile uint32_t *)0x00000000u))
#define REG_UART_0_STATUS (*((volatile uint32_t *)0x00000004u))

/* [必须修改] TODO_BSP_REQUIRED：根据芯片手册填写 GPIO 寄存器。 */
#define REG_GPIO_0_DIR    (*((volatile uint32_t *)0x00000000u))
#define REG_GPIO_0_INPUT  (*((volatile uint32_t *)0x00000004u))
#define REG_GPIO_0_OUTPUT (*((volatile uint32_t *)0x00000008u))

/* [按需修改] 增加该板卡实际存在的其他寄存器和引脚定义。 */

#endif
