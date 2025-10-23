#ifndef __I2C_TYPE_H__
#define __I2C_TYPE_H__

#include <stdint.h>

/* ========== I2C 接口寄存器 ========== */
#define REG_CUST_I2C_CTRL     (*(volatile uint32_t*)0x03006000)  // I2C控制寄存器
#define REG_CUST_I2C_PSCR     (*(volatile uint32_t*)0x03006004)  // I2C预分频寄存器
#define REG_CUST_I2C_TXR      (*(volatile uint32_t*)0x03006008)  // I2C发送寄存器
#define REG_CUST_I2C_RXR      (*(volatile uint32_t*)0x0300600c)  // I2C接收寄存器
#define REG_CUST_I2C_CMD      (*(volatile uint32_t*)0x03006010)  // I2C命令寄存器
#define REG_CUST_I2C_SR       (*(volatile uint32_t*)0x03006014)  // I2C状态寄存器

// I2C状态寄存器位定义
#define I2C_STATUS_RXACK     ((uint32_t)0x80) // (1 << 7)
#define I2C_STATUS_BUSY      ((uint32_t)0x40) // (1 << 6)
#define I2C_STATUS_AL        ((uint32_t)0x20) // (1 << 5)
#define I2C_STATUS_TIP       ((uint32_t)0x02) // (1 << 1)
#define I2C_STATUS_IF        ((uint32_t)0x01) // (1 << 0)

// i2c 配置结构体
typedef struct {
    uint32_t pscr; // I2C预分频寄存器值
} i2c_config_t;

// i2c 寄存器地址长度枚举
typedef enum {
    I2C_REG_8 = 8,
    I2C_REG_16 = 16,
} i2c_reg_addr_len_t;

// i2c 命令枚举
typedef enum {
    I2C_START = ((uint32_t)0x80),
    I2C_STOP = ((uint32_t)0x40),
    I2C_READ = ((uint32_t)0x20),
    I2C_WRITE = ((uint32_t)0x10),
    I2C_START_READ = ((uint32_t)0xA0),
    I2C_START_WRITE = ((uint32_t)0x90),
    I2C_STOP_READ = ((uint32_t)0x60),
    I2C_STOP_WRITE = ((uint32_t)0x50),
} i2c_cmd_t;

#endif