#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#include "hal_i2c_type.h"

int hal_i2c_init(hal_i2c_port_t port, const hal_i2c_config_t *config);
int hal_i2c_deinit(hal_i2c_port_t port);

int hal_i2c_write_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, const uint8_t* data, uint32_t len);
int hal_i2c_read_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, uint8_t* data, uint32_t len);

#endif