#ifndef ECOS_SOC_YSYX_2512_H
#define ECOS_SOC_YSYX_2512_H

#include <stdint.h>

/* ========================== GPIO_0 寄存器组 ================================ */
#define REG_GPIO_0_PADDIR               (*((volatile uint32_t *)0x10100000))
#define REG_GPIO_0_PADIN                (*((volatile uint32_t *)0x10100004))
#define REG_GPIO_0_PADOUT               (*((volatile uint32_t *)0x10100008))
#define REG_GPIO_0_INTEN                (*((volatile uint32_t *)0x1010000C))
#define REG_GPIO_0_INTTYPE0             (*((volatile uint32_t *)0x10100010))
#define REG_GPIO_0_INTTYPE1             (*((volatile uint32_t *)0x10100014))
#define REG_GPIO_0_INTSTAT              (*((volatile uint32_t *)0x10100018))
#define REG_GPIO_0_IOFCFG               (*((volatile uint32_t *)0x1010001C))
#define REG_GPIO_0_PINMUX               (*((volatile uint32_t *)0x10100020))

/* ========================== GPIO_1 寄存器组 ================================ */
#define GPIO_GROUP_1                    1
#define REG_GPIO_1_PADDIR               (*((volatile uint32_t *)0x10101000))
#define REG_GPIO_1_PADIN                (*((volatile uint32_t *)0x10101004))
#define REG_GPIO_1_PADOUT               (*((volatile uint32_t *)0x10101008))
#define REG_GPIO_1_INTEN                (*((volatile uint32_t *)0x1010100C))
#define REG_GPIO_1_INTTYPE0             (*((volatile uint32_t *)0x10101010))
#define REG_GPIO_1_INTTYPE1             (*((volatile uint32_t *)0x10101014))
#define REG_GPIO_1_INTSTAT              (*((volatile uint32_t *)0x10101018))
#define REG_GPIO_1_IOFCFG               (*((volatile uint32_t *)0x1010101C))
#define REG_GPIO_1_PINMUX               (*((volatile uint32_t *)0x10101020))

/* ========================== GPIO_2/PINCTRL 寄存器组 ======================== */
#define GPIO_GROUP_2                    2
#define REG_GPIO_2_PADDIR               (*((volatile uint32_t *)0x10102000))
#define REG_GPIO_2_PADIN                (*((volatile uint32_t *)0x10102004))
#define REG_GPIO_2_PADOUT               (*((volatile uint32_t *)0x10102008))
#define REG_GPIO_2_INTEN                (*((volatile uint32_t *)0x1010200C))
#define REG_GPIO_2_INTTYPE0             (*((volatile uint32_t *)0x10102010))
#define REG_GPIO_2_INTTYPE1             (*((volatile uint32_t *)0x10102014))
#define REG_GPIO_2_INTSTAT              (*((volatile uint32_t *)0x10102018))
#define REG_GPIO_2_IOFCFG               (*((volatile uint32_t *)0x1010201C))
#define REG_GPIO_2_PINMUX               (*((volatile uint32_t *)0x10102020))

/* ========================== SYS_UART 寄存器组 ================================ */
#define REG_UART_0_RB                   (*((volatile uint8_t *)0x10000000))
#define REG_UART_0_TH                   (*((volatile uint8_t *)0x10000000))
#define REG_UART_0_DATA                 (*((volatile uint8_t *)0x10000000))
#define REG_UART_0_IE                   (*((volatile uint8_t *)0x10000001))
#define REG_UART_0_II                   (*((volatile uint8_t *)0x10000002))
#define REG_UART_0_FC                   (*((volatile uint8_t *)0x10000002))
#define REG_UART_0_LC                   (*((volatile uint8_t *)0x10000003))
#define REG_UART_0_MC                   (*((volatile uint8_t *)0x10000004))
#define REG_UART_0_LS                   (*((volatile uint8_t *)0x10000005))
#define REG_UART_0_MS                   (*((volatile uint8_t *)0x10000006))

/* ========================== UART 寄存器组 ================================ */
#define REG_UART_1_LCR                  (*((volatile uint32_t*)0x10103000))
#define REG_UART_1_DIV                  (*((volatile uint32_t*)0x10103004))
#define REG_UART_1_TRX                  (*((volatile uint32_t*)0x10103008))
#define REG_UART_1_FCR                  (*((volatile uint32_t*)0x1010300C))
#define REG_UART_1_LSR                  (*((volatile uint32_t*)0x10103010))

/* ========================== QSPI 寄存器组 ================================ */
#define REG_QSPI_0_STATUS               (*((volatile uint32_t *)0x10200000))
#define REG_QSPI_0_CLKDIV               (*((volatile uint32_t *)0x10200004))
#define REG_QSPI_0_CMD                  (*((volatile uint32_t *)0x10200008))
#define REG_QSPI_0_ADR                  (*((volatile uint32_t *)0x1020000C))
#define REG_QSPI_0_LEN                  (*((volatile uint32_t *)0x10200010))
#define REG_QSPI_0_DUM                  (*((volatile uint32_t *)0x10200014))
#define REG_QSPI_0_TXFIFO               (*((volatile uint32_t *)0x10200018))
#define REG_QSPI_0_RXFIFO               (*((volatile uint32_t *)0x10200020))
#define REG_QSPI_0_INTCFG               (*((volatile uint32_t *)0x10200024))
#define REG_QSPI_0_INTSTA               (*((volatile uint32_t *)0x10200028))

/* ========================== I2C 寄存器组 ================================ */
#define REG_I2C_0_CTRL                  (*((volatile uint8_t *)0x10104000))
#define REG_I2C_0_PSCR                  (*((volatile uint8_t *)0x10104004))
#define REG_I2C_0_TXR                   (*((volatile uint8_t *)0x10104008))
#define REG_I2C_0_RXR                   (*((volatile uint8_t *)0x1010400C))
#define REG_I2C_0_CMD                   (*((volatile uint8_t *)0x10104010))
#define REG_I2C_0_SR                    (*((volatile uint8_t *)0x10104014))

/* ========================== TIMER_0 寄存器组 ================================ */
#define REG_TIMER_0_CTRL                (*((volatile uint32_t *)0x10108000))
#define REG_TIMER_0_PSCR                (*((volatile uint32_t *)0x10108004))
#define REG_TIMER_0_CNT                 (*((volatile uint32_t *)0x10108008))
#define REG_TIMER_0_CMP                 (*((volatile uint32_t *)0x1010800C))
#define REG_TIMER_0_STAT                (*((volatile uint32_t *)0x10108010))

/* ========================== TIMER_1 寄存器组 ================================ */
#define REG_TIMER_1_CTRL                (*((volatile uint32_t *)0x10109000))
#define REG_TIMER_1_PSCR                (*((volatile uint32_t *)0x10109004))
#define REG_TIMER_1_CNT                 (*((volatile uint32_t *)0x10109008))
#define REG_TIMER_1_CMP                 (*((volatile uint32_t *)0x1010900C))
#define REG_TIMER_1_STAT                (*((volatile uint32_t *)0x10109010))

/* ========================== TIMER_2 寄存器组 ================================ */
#define REG_TIMER_2_CTRL                (*((volatile uint32_t *)0x1010A000))
#define REG_TIMER_2_PSCR                (*((volatile uint32_t *)0x1010A004))
#define REG_TIMER_2_CNT                 (*((volatile uint32_t *)0x1010A008))
#define REG_TIMER_2_CMP                 (*((volatile uint32_t *)0x1010A00C))
#define REG_TIMER_2_STAT                (*((volatile uint32_t *)0x1010A010))

/* ========================== TIMER_3 寄存器组 ================================ */
#define REG_TIMER_3_CTRL                (*((volatile uint32_t *)0x1010B000))
#define REG_TIMER_3_PSCR                (*((volatile uint32_t *)0x1010B004))
#define REG_TIMER_3_CNT                 (*((volatile uint32_t *)0x1010B008))
#define REG_TIMER_3_CMP                 (*((volatile uint32_t *)0x1010B00C))
#define REG_TIMER_3_STAT                (*((volatile uint32_t *)0x1010B010))

/* ========================== PWM_0 寄存器组 ================================ */
#define REG_PWM_0_CTRL                  (*((volatile uint32_t *)0x10106000))
#define REG_PWM_0_PSCR                  (*((volatile uint32_t *)0x10106004))
#define REG_PWM_0_CNT                   (*((volatile uint32_t *)0x10106008))
#define REG_PWM_0_CMP                   (*((volatile uint32_t *)0x1010600C))
#define REG_PWM_0_CR0                   (*((volatile uint32_t *)0x10106010))
#define REG_PWM_0_CR1                   (*((volatile uint32_t *)0x10106014))
#define REG_PWM_0_CR2                   (*((volatile uint32_t *)0x10106018))
#define REG_PWM_0_CR3                   (*((volatile uint32_t *)0x1010601C))
#define REG_PWM_0_STAT                  (*((volatile uint32_t *)0x10106020))

/* ========================== PWM_1 寄存器组 ================================ */
#define REG_PWM_1_CTRL                  (*((volatile uint32_t *)0x10107000))
#define REG_PWM_1_PSCR                  (*((volatile uint32_t *)0x10107004))
#define REG_PWM_1_CNT                   (*((volatile uint32_t *)0x10107008))
#define REG_PWM_1_CMP                   (*((volatile uint32_t *)0x1010700C))
#define REG_PWM_1_CR0                   (*((volatile uint32_t *)0x10107010))
#define REG_PWM_1_CR1                   (*((volatile uint32_t *)0x10107014))
#define REG_PWM_1_CR2                   (*((volatile uint32_t *)0x10107018))
#define REG_PWM_1_CR3                   (*((volatile uint32_t *)0x1010701C))
#define REG_PWM_1_STAT                  (*((volatile uint32_t *)0x10107020))

/* ========================== PS2 寄存器组 ================================ */
#define REG_PS2_0_CTRL                  (*((volatile uint32_t *)0x10105000))
#define REG_PS2_0_DATA                  (*((volatile uint32_t *)0x10105004))
#define REG_PS2_0_STAT                  (*((volatile uint32_t *)0x10105008))

/* ========================== WDG 寄存器组 ================================ */
#define REG_WDG_0_CTRL                  (*((volatile uint32_t *)0x10005000))
#define REG_WDG_0_PSCR                  (*((volatile uint32_t *)0x10005004))
#define REG_WDG_0_CNT                   (*((volatile uint32_t *)0x10005008))
#define REG_WDG_0_CMP                   (*((volatile uint32_t *)0x1000500C))
#define REG_WDG_0_STAT                  (*((volatile uint32_t *)0x10005010))
#define REG_WDG_0_KEY                   (*((volatile uint32_t *)0x10005014))
#define REG_WDG_0_FEED                  (*((volatile uint32_t *)0x10005018))

/* ========================== RTC 寄存器组 ================================ */
#define REG_RTC_0_CTRL                  (*((volatile uint32_t *)0x10004000))
#define REG_RTC_0_PSCR                  (*((volatile uint32_t *)0x10004004))
#define REG_RTC_0_CNT                   (*((volatile uint32_t *)0x10004008))
#define REG_RTC_0_ALRM                  (*((volatile uint32_t *)0x1000400C))
#define REG_RTC_0_ISTA                  (*((volatile uint32_t *)0x10004010))
#define REG_RTC_0_SSTA                  (*((volatile uint32_t *)0x10004014))

/* ========================== RCU 寄存器组 ================================ */
#define REG_RCU_0_CTRL                  (*((volatile uint32_t *)0x10002000))
#define REG_RCU_0_RDIV                  (*((volatile uint32_t *)0x10002004))
#define REG_RCU_0_STAT                  (*((volatile uint32_t *)0x10002008))

/* ========================== ARCHINFO 寄存器组 ================================ */
#define REG_ARCHINFO_0_SYS              (*((volatile uint32_t *)0x10006000))
#define REG_ARCHINFO_0_IDL              (*((volatile uint32_t *)0x10006004))
#define REG_ARCHINFO_0_IDH              (*((volatile uint32_t *)0x10006008))

/* ========================== RNG 寄存器组 ================================ */
#define REG_RNG_0_CTRL                  (*((volatile uint32_t *)0x10300000))
#define REG_RNG_0_SEED                  (*((volatile uint32_t *)0x10300004))
#define REG_RNG_0_VAL                   (*((volatile uint32_t *)0x10300008))

/* ========================== CRC 寄存器组 ================================ */
#define REG_CRC_0_CTRL                  (*((volatile uint32_t *)0x10301000))
#define REG_CRC_0_INIT                  (*((volatile uint32_t *)0x10301004))
#define REG_CRC_0_XORV                  (*((volatile uint32_t *)0x10301008))
#define REG_CRC_0_DATA                  (*((volatile uint32_t *)0x1030100C))
#define REG_CRC_0_STAT                  (*((volatile uint32_t *)0x10301010))

#endif
