#ifndef __HAL_I2C_TYPE_H__
#define __HAL_I2C_TYPE_H__

#include <stdint.h>

#define I2C_STATUS_RXACK     ((uint32_t)0x80)
#define I2C_STATUS_BUSY      ((uint32_t)0x40)
#define I2C_STATUS_AL        ((uint32_t)0x20)
#define I2C_STATUS_TIP       ((uint32_t)0x02)
#define I2C_STATUS_IF        ((uint32_t)0x01)

typedef struct{
    uint32_t pscr;
} i2c_config_t;

typedef enum{
    I2C_REG_8  = 8,
    I2C_REG_16 = 16,
} i2c_reg_addr_len_t;

typedef enum{
    I2C_START       = 0x80,
    I2C_STOP        = 0x40,
    I2C_READ        = 0x20,
    I2C_WRITE       = 0x10,
    I2C_START_READ  = 0xA0,
    I2C_START_WRITE = 0x90,
    I2C_STOP_READ   = 0x60,
    I2C_STOP_WRITE  = 0x50,
} i2c_cmd_t;

#endif