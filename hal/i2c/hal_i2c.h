#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#include <stdint.h>
#include <stddef.h>

typedef enum {
    HAL_I2C_PORT_0 = 0,
    HAL_I2C_PORT_MAX
} hal_i2c_port_t;

typedef enum {
    HAL_I2C_REG_8 = 8,
    HAL_I2C_REG_16 = 16,
} hal_i2c_reg_addr_len_t;

typedef struct {
    uint32_t pscr; // I2C预分频寄存器值
} hal_i2c_config_t;

int hal_i2c_init(hal_i2c_port_t port, const hal_i2c_config_t *config);
int hal_i2c_deinit(hal_i2c_port_t port);

int hal_i2c_write_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, const uint8_t* data, uint32_t len);
int hal_i2c_read_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, uint8_t* data, uint32_t len);

#endif // __HAL_I2C_H__