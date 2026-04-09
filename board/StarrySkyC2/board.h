#ifndef __STARRYSKY_C2_H__
#define __STARRYSKY_C2_H__

/* ========== GPIO 寄存器组 ========== */
#define REG_GPIO_0_DR         (*(volatile uint32_t*)0x10000000)  // GPIO数据寄存器
#define REG_GPIO_0_DDR        (*(volatile uint32_t*)0x10000004)  // GPIO数据方向寄存器
#define REG_GPIO_0_PUB        (*(volatile uint32_t*)0x10000008)  // GPIO上拉使能寄存器
#define REG_GPIO_0_PDB        (*(volatile uint32_t*)0x1000000c)  // GPIO下拉使能寄存器

/* ========== SYS_UART 接口寄存器 ====== */
#define REG_UART_0_CLKDIV     (*(volatile uint32_t*)0x10001000)  // UART时钟分频寄存器
#define REG_UART_0_DATA       (*(volatile uint32_t*)0x10001004)  // UART数据寄存器

/* ========== 定时器 寄存器组 =========== */
#define REG_TIM_0_CONFIG      (*(volatile uint32_t*)0x10002000)  // 定时器0配置寄存器
#define REG_TIM_0_VALUE       (*(volatile uint32_t*)0x10002004)  // 定时器0计数值寄存器
#define REG_TIM_0_DATA        (*(volatile uint32_t*)0x10002008)  // 定时器0数据寄存器

#define REG_TIM_1_CONFIG      (*(volatile uint32_t*)0x10003000)  // 定时器1配置寄存器
#define REG_TIM_1_VALUE       (*(volatile uint32_t*)0x10003004)  // 定时器1计数值寄存器
#define REG_TIM_1_DATA        (*(volatile uint32_t*)0x10003008)  // 定时器1数据寄存器

/* ========== PSRAM  寄存器组 ========== */
#define REG_PSRAM_0_WC         (*(volatile uint32_t*)0x10004000)  // 
#define REG_PSRAM_0_CHD        (*(volatile uint32_t*)0x10004004)  // 

/* ======== ARCHINFO 寄存器组 ======== */
#define REG_ARCHINFO_0_SYS     (*(volatile uint32_t*)0x20001000)  // 系统架构信息寄存器
#define REG_ARCHINFO_0_IDL     (*(volatile uint32_t*)0x20001004)  // 系统架构信息寄存器
#define REG_ARCHINFO_0_IDH     (*(volatile uint32_t*)0x20001008)  // 系统架构信息寄存器

/* =========== RNG 寄存器组 ========== */
#define REG_RNG_0_CTRL         (*(volatile uint32_t*)0x20002000)  // RNG控制寄存器
#define REG_RNG_0_SEED         (*(volatile uint32_t*)0x20002004)  // RNG种子寄存器
#define REG_RNG_0_VAL          (*(volatile uint32_t*)0x20002008)  // RNG随机数寄存器

/* ========== HP_UART 寄存器组 ======= */
#define REG_UART_1_LCR        (*(volatile uint32_t*)0x20003000)  // UART线路控制寄存器
#define REG_UART_1_DIV        (*(volatile uint32_t*)0x20003004)  // UART分频寄存器
#define REG_UART_1_TRX        (*(volatile uint32_t*)0x20003008)  // UART收发寄存器
#define REG_UART_1_FCR        (*(volatile uint32_t*)0x2000300c)  // UART FIFO控制寄存器
#define REG_UART_1_LSR        (*(volatile uint32_t*)0x20003010)  // UART线路状态寄存器

/* ========== PWM 寄存器组 ========== */
#define REG_PWM_0_CTRL        (*(volatile uint32_t*)0x20004000)  // PWM控制寄存器
#define REG_PWM_0_PSCR        (*(volatile uint32_t*)0x20004004)  // PWM预分频寄存器
#define REG_PWM_0_CNT         (*(volatile uint32_t*)0x20004008)  // PWM计数寄存器
#define REG_PWM_0_CMP         (*(volatile uint32_t*)0x2000400c)  // PWM比较寄存器
#define REG_PWM_0_CR0         (*(volatile uint32_t*)0x20004010)  // PWM通道0比较寄存器
#define REG_PWM_0_CR1         (*(volatile uint32_t*)0x20004014)  // PWM通道1比较寄存器
#define REG_PWM_0_CR2         (*(volatile uint32_t*)0x20004018)  // PWM通道2比较寄存器
#define REG_PWM_0_CR3         (*(volatile uint32_t*)0x2000401c)  // PWM通道3比较寄存器
#define REG_PWM_0_STAT        (*(volatile uint32_t*)0x20004020)  // PWM状态寄存器

/* ========== CUST_PS2 寄存器组 ======== */
#define REG_PS2_0_CTRL        (*(volatile uint32_t*)0x20005000)  // PS2控制寄存器
#define REG_PS2_0_DATA        (*(volatile uint32_t*)0x20005004)  // PS2数据寄存器
#define REG_PS2_0_STAT        (*(volatile uint32_t*)0x20005008)  // PS2状态寄存器

/* ========== I2C 接口寄存器 ========== */
#define REG_I2C_0_CTRL        (*(volatile uint32_t*)0x20006000)  // I2C控制寄存器
#define REG_I2C_0_PSCR        (*(volatile uint32_t*)0x20006004)  // I2C预分频寄存器
#define REG_I2C_0_TXR         (*(volatile uint32_t*)0x20006008)  // I2C发送寄存器
#define REG_I2C_0_RXR         (*(volatile uint32_t*)0x2000600c)  // I2C接收寄存器
#define REG_I2C_0_CMD         (*(volatile uint32_t*)0x20006010)  // I2C命令寄存器
#define REG_I2C_0_SR          (*(volatile uint32_t*)0x20006014)  // I2C状态寄存器

/* ========== QSPI 接口寄存器 ========== */
#define REG_QSPI_0_STATUS     (*(volatile uint32_t*)0x20007000)  // QSPI状态寄存器
#define REG_QSPI_0_CLKDIV     (*(volatile uint32_t*)0x20007004)  // QSPI时钟分频寄存器
#define REG_QSPI_0_CMD        (*(volatile uint32_t*)0x20007008)  // QSPI命令寄存器
#define REG_QSPI_0_ADR        (*(volatile uint32_t*)0x2000700c)  // QSPI地址寄存器
#define REG_QSPI_0_LEN        (*(volatile uint32_t*)0x20007010)  // QSPI长度寄存器
#define REG_QSPI_0_DUM        (*(volatile uint32_t*)0x20007014)  // QSPI虚拟周期寄存器
#define REG_QSPI_0_TXFIFO     (*(volatile uint32_t*)0x20007018)  // QSPI发送FIFO寄存器
#define REG_QSPI_0_RXFIFO     (*(volatile uint32_t*)0x20007020)  // QSPI接收FIFO寄存器
#define REG_QSPI_0_INTCFG     (*(volatile uint32_t*)0x20007024)  // QSPI中断配置寄存器
#define REG_QSPI_0_INTSTA     (*(volatile uint32_t*)0x20007028)  // QSPI中断状态寄存器

#endif
