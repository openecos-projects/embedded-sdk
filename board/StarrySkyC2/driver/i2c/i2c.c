#include "hal_i2c.h"
#include "hal_i2c_type.h"
#include "generated/autoconf.h"
#include "board.h"
#include "hal_gpio.h"

void hal_i2c_init(i2c_config_t *config){
#ifdef CONFIG_STARRYSKY_L3
    hal_gpio_set_function(GPIO_NUM_29, GPIO_FUNCTION_1);            //根据具体实际情况改
    hal_gpio_set_function(GPIO_NUM_30, GPIO_FUNCTION_1);            //根据
#endif

    REG_I2C_0_CTRL = 0;
    REG_I2C_0_PSCR = config->pscr;
    REG_I2C_0_CTRL = 0x80;
}

static uint32_t hal_i2c_get_ack(void){
    while ((REG_I2C_0_SR & I2C_STATUS_TIP) == 0);
    while ((REG_I2C_0_SR & I2C_STATUS_TIP) != 0);
    return !(REG_I2C_0_SR & I2C_STATUS_RXACK);
}

static uint32_t hal_i2c_busy(void){
    return ((REG_I2C_0_SR & I2C_STATUS_BUSY) == I2C_STATUS_BUSY);
}

static void hal_i2c_start_write(uint8_t slave_addr){
    do {
        REG_I2C_0_TXR = slave_addr << 1;
        REG_I2C_0_CMD = I2C_START_WRITE;
    } while (!hal_i2c_get_ack());
}

static void hal_i2c_start_read(uint8_t slave_addr){
    REG_I2C_0_TXR = (slave_addr << 1) | 0x01;
    REG_I2C_0_CMD = I2C_START_WRITE;
    hal_i2c_get_ack();
}

static void hal_i2c_stop(void){
    REG_I2C_0_CMD = I2C_STOP;
    while (hal_i2c_busy());
}

static void hal_i2c_write_byte(uint8_t byte){
    REG_I2C_0_TXR = byte;
    REG_I2C_0_CMD = I2C_WRITE;
    hal_i2c_get_ack();
}

static uint32_t hal_i2c_read_byte(i2c_cmd_t cmd){
    REG_I2C_0_CMD = cmd;
    hal_i2c_get_ack();
    return REG_I2C_0_RXR;
}

void hal_i2c_write_nbyte(uint8_t slave_addr, uint16_t reg_addr, i2c_reg_addr_len_t reg_addr_len, uint8_t *data, uint32_t len){
    hal_i2c_start_write(slave_addr);

    if (reg_addr_len == I2C_REG_8) {
        hal_i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    } else {
        hal_i2c_write_byte((uint8_t)(reg_addr >> 8));
        hal_i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    }

    for (uint32_t i = 0; i < len; i++) {
        hal_i2c_write_byte(data[i]);
    }

    hal_i2c_stop();
}

void hal_i2c_read_nbyte(uint8_t slave_addr, uint16_t reg_addr, i2c_reg_addr_len_t reg_addr_len, uint8_t *data, uint32_t len){
    hal_i2c_start_write(slave_addr);

    if (reg_addr_len == I2C_REG_8) {
        hal_i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    } else {
        hal_i2c_write_byte((uint8_t)(reg_addr >> 8));
        hal_i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    }

    hal_i2c_stop();
    hal_i2c_start_read(slave_addr);

    for (uint32_t i = 0; i < len; i++) {
        data[i] = (uint8_t)hal_i2c_read_byte(
            (i == len - 1) ? I2C_STOP_READ : I2C_READ);
    }

    hal_i2c_stop();
}