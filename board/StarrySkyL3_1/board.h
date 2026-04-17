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

#endif
