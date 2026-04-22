#ifndef __HAL_I2C_TYPE_H__
#define __HAL_I2C_TYPE_H__

#include <stdint.h>
#include <stddef.h>

typedef enum{
    HAL_I2C_PORT_0 = 0,
    HAL_I2C_PORT_MAX
} hal_i2c_port_t;

typedef enum{
    HAL_I2C_REG_8  = 8,
    HAL_I2C_REG_16 = 16,
} hal_i2c_reg_addr_len_t;

typedef struct{
    uint32_t pscr;
} hal_i2c_config_t;

#endif 