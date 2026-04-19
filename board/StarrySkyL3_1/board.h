#ifndef __STARRYSKY_L3_H__
#define __STARRYSKY_L3_H__

/* ========================== GPIO_0 寄存器组 ================================ */
#define REG_GPIO_0_PADDIR               (*((volatile uint32_t *)0x10002000))
#define REG_GPIO_0_PADIN                (*((volatile uint32_t *)0x10002004))
#define REG_GPIO_0_PADOUT               (*((volatile uint32_t *)0x10002008))
#define REG_GPIO_0_INTEN                (*((volatile uint32_t *)0x1000200C))
#define REG_GPIO_0_INTTYPE0             (*((volatile uint32_t *)0x10002010))
#define REG_GPIO_0_INTTYPE1             (*((volatile uint32_t *)0x10002014))
#define REG_GPIO_0_INTSTAT              (*((volatile uint32_t *)0x10002018))
#define REG_GPIO_0_IOFCFG               (*((volatile uint32_t *)0x1000201C))
#define REG_GPIO_0_PINMUX               (*((volatile uint32_t *)0x10002020))

/* ========================== GPIO_0 寄存器组 ================================ */
#define GPIO_GROUP_1                    1
#define REG_GPIO_1_PADDIR               (*((volatile uint32_t *)0x10003000))
#define REG_GPIO_1_PADIN                (*((volatile uint32_t *)0x10003004))
#define REG_GPIO_1_PADOUT               (*((volatile uint32_t *)0x10003008))
#define REG_GPIO_1_INTEN                (*((volatile uint32_t *)0x1000300C))
#define REG_GPIO_1_INTTYPE0             (*((volatile uint32_t *)0x10003010))
#define REG_GPIO_1_INTTYPE1             (*((volatile uint32_t *)0x10003014))
#define REG_GPIO_1_INTSTAT              (*((volatile uint32_t *)0x10003018))
#define REG_GPIO_1_IOFCFG               (*((volatile uint32_t *)0x1000301C))
#define REG_GPIO_1_PINMUX               (*((volatile uint32_t *)0x10003020))

/* ========================== SYS_UART 寄存器组 ================================ */
#define REG_UART_0_RB                   (*((volatile uint8_t *)0x10000000))
#define REG_UART_0_TH                   (*((volatile uint8_t *)0x10000000))
#define REG_UART_0_IE                   (*((volatile uint8_t *)0x10000001))
#define REG_UART_0_II                   (*((volatile uint8_t *)0x10000002))
#define REG_UART_0_FC                   (*((volatile uint8_t *)0x10000002))
#define REG_UART_0_LC                   (*((volatile uint8_t *)0x10000003))
#define REG_UART_0_MC                   (*((volatile uint8_t *)0x10000004))
#define REG_UART_0_LS                   (*((volatile uint8_t *)0x10000005))
#define REG_UART_0_MS                   (*((volatile uint8_t *)0x10000006))

/* ========================== QSPI 寄存器组 ================================ */
#define REG_QSPI_0_STATUS               (*((volatile uint32_t *)0x10010000))
#define REG_QSPI_0_CLKDIV               (*((volatile uint32_t *)0x10010004))
#define REG_QSPI_0_CMD                  (*((volatile uint32_t *)0x10010008))
#define REG_QSPI_0_ADR                  (*((volatile uint32_t *)0x1001000C))
#define REG_QSPI_0_LEN                  (*((volatile uint32_t *)0x10010010))
#define REG_QSPI_0_DUM                  (*((volatile uint32_t *)0x10010014))
#define REG_QSPI_0_TXFIFO               (*((volatile uint32_t *)0x10010018))
#define REG_QSPI_0_RXFIFO               (*((volatile uint32_t *)0x10010020))
#define REG_QSPI_0_INTCFG               (*((volatile uint32_t *)0x10010024))
#define REG_QSPI_0_INTSTA               (*((volatile uint32_t *)0x10010028))

/* ========================== I2C 寄存器组 ================================ */
#define REG_I2C_0_CTRL                  (*((volatile uint8_t *)0x1000D000))
#define REG_I2C_0_PSCR                  (*((volatile uint8_t *)0x1000D004))
#define REG_I2C_0_TXR                   (*((volatile uint8_t *)0x1000D008))
#define REG_I2C_0_RXR                   (*((volatile uint8_t *)0x1000D00C))
#define REG_I2C_0_CMD                   (*((volatile uint8_t *)0x1000D010))
#define REG_I2C_0_SR                    (*((volatile uint8_t *)0x1000D014))   

/* ========================== TIMER_0 寄存器组 ================================ */
#define REG_TIMER_0_CTRL                (*((volatile uint32_t *)0x10009000))
#define REG_TIMER_0_PSCR                (*((volatile uint32_t *)0x10009004))
#define REG_TIMER_0_CNT                 (*((volatile uint32_t *)0x10009008))
#define REG_TIMER_0_CMP                 (*((volatile uint32_t *)0x1000900C))
#define REG_TIMER_0_STAT                (*((volatile uint32_t *)0x10009010))

/* ========================== PWM_0 寄存器组 ================================ */
#define REG_PWM_0_CTRL                  (*((volatile uint32_t *)0x10004000))
#define REG_PWM_0_PSCR                  (*((volatile uint32_t *)0x10004004))
#define REG_PWM_0_CNT                   (*((volatile uint32_t *)0x10004008))
#define REG_PWM_0_CMP                   (*((volatile uint32_t *)0x1000400C))
#define REG_PWM_0_CR0                   (*((volatile uint32_t *)0x10004010))
#define REG_PWM_0_CR1                   (*((volatile uint32_t *)0x10004014))
#define REG_PWM_0_CR2                   (*((volatile uint32_t *)0x10004018))
#define REG_PWM_0_CR3                   (*((volatile uint32_t *)0x1000401C))
#define REG_PWM_0_STAT                  (*((volatile uint32_t *)0x10004020))

/* ========================== PWM_1 寄存器组 ================================ */
#define REG_PWM_1_CTRL                  (*((volatile uint32_t *)0x10005000))
#define REG_PWM_1_PSCR                  (*((volatile uint32_t *)0x10005004))
#define REG_PWM_1_CNT                   (*((volatile uint32_t *)0x10005008))
#define REG_PWM_1_CMP                   (*((volatile uint32_t *)0x1000500C))
#define REG_PWM_1_CR0                   (*((volatile uint32_t *)0x10005010))
#define REG_PWM_1_CR1                   (*((volatile uint32_t *)0x10005014))
#define REG_PWM_1_CR2                   (*((volatile uint32_t *)0x10005018))
#define REG_PWM_1_CR3                   (*((volatile uint32_t *)0x1000501C))
#define REG_PWM_1_STAT                  (*((volatile uint32_t *)0x10005020))

/* ========================== PWM_2 寄存器组 ================================ */
#define REG_PWM_2_CTRL                  (*((volatile uint32_t *)0x10006000))
#define REG_PWM_2_PSCR                  (*((volatile uint32_t *)0x10006004))
#define REG_PWM_2_CNT                   (*((volatile uint32_t *)0x10006008))
#define REG_PWM_2_CMP                   (*((volatile uint32_t *)0x1000600C))
#define REG_PWM_2_CR0                   (*((volatile uint32_t *)0x10006010))
#define REG_PWM_2_CR1                   (*((volatile uint32_t *)0x10006014))
#define REG_PWM_2_CR2                   (*((volatile uint32_t *)0x10006018))
#define REG_PWM_2_CR3                   (*((volatile uint32_t *)0x1000601C))
#define REG_PWM_2_STAT                  (*((volatile uint32_t *)0x10006020))

/* ========================== PS2 寄存器组 ================================ */
#define REG_PS2_0_CTRL                  (*((volatile uint32_t *)0x10013000))
#define REG_PS2_0_DATA                  (*((volatile uint32_t *)0x10013004))
#define REG_PS2_0_STAT                  (*((volatile uint32_t *)0x10013008))

/* ========================== WDG 寄存器组 ================================ */
#define REG_WDG_0_CTRL                  (*((volatile uint32_t *)0x10008000))
#define REG_WDG_0_PSCR                  (*((volatile uint32_t *)0x10008004))
#define REG_WDG_0_CNT                   (*((volatile uint32_t *)0x10008008))
#define REG_WDG_0_CMP                   (*((volatile uint32_t *)0x1000800C))
#define REG_WDG_0_STAT                  (*((volatile uint32_t *)0x10008010))
#define REG_WDG_0_KEY                   (*((volatile uint32_t *)0x10008014))
#define REG_WDG_0_FEED                  (*((volatile uint32_t *)0x10008018))

/* ========================== RTC 寄存器组 ================================ */
#define REG_RTC_0_CTRL                  (*((volatile uint32_t *)0x10007000))
#define REG_RTC_0_PSCR                  (*((volatile uint32_t *)0x10007004))
#define REG_RTC_0_CNT                   (*((volatile uint32_t *)0x10007008))
#define REG_RTC_0_ALRM                  (*((volatile uint32_t *)0x1000700C))
#define REG_RTC_0_ISTA                  (*((volatile uint32_t *)0x10007010))
#define REG_RTC_0_SSTA                  (*((volatile uint32_t *)0x10007014))

/* ========================== RCU 寄存器组 ================================ */
#define REG_RCU_0_CTRL                  (*((volatile uint32_t *)0x10018000))
#define REG_RCU_0_RDIV                  (*((volatile uint32_t *)0x10018004))
#define REG_RCU_0_STAT                  (*((volatile uint32_t *)0x10018008))

/* ========================== ARCHINFO 寄存器组 ================================ */
#define REG_ARCHINFO_0_SYS              (*((volatile uint32_t *)0x10017000))
#define REG_ARCHINFO_0_IDL              (*((volatile uint32_t *)0x10017004))
#define REG_ARCHINFO_0_IDH              (*((volatile uint32_t *)0x10017008))

/* ========================== RNG 寄存器组 ================================ */
#define REG_RNG_0_CTRL                  (*((volatile uint32_t *)0x10014000))
#define REG_RNG_0_SEED                  (*((volatile uint32_t *)0x10014004))
#define REG_RNG_0_VAL                   (*((volatile uint32_t *)0x10014008))

/* ========================== CRC 寄存器组 ================================ */
#define REG_CRC_0_CTRL                  (*((volatile uint32_t *)0x10015000))
#define REG_CRC_0_INIT                  (*((volatile uint32_t *)0x10015004))
#define REG_CRC_0_XORV                  (*((volatile uint32_t *)0x10015008))
#define REG_CRC_0_DATA                  (*((volatile uint32_t *)0x1001500C))
#define REG_CRC_0_STAT                  (*((volatile uint32_t *)0x10015010))

#endif
