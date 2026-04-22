#include "hal_i2c.h"
#include "hal_i2c_type.h"
#include "generated/autoconf.h"
#include "board.h"
#include "hal_gpio.h"

#define I2C_START_WRITE_CMD     0x90
#define I2C_WRITE_BYTE_CMD     0x10
#define I2C_DATA_READ_CMD       0x20    
#define I2C_LAST_READ_CMD       0x60
#define I2C_STOP_CMD            0x40

#define I2C_STATUS_TIP      (1 << 1)
#define I2C_STATUS_RXACK    (1 << 7)
#define I2C_STATUS_BUSY     (1 << 6)

typedef enum{
    I2C_CMD_NORMAL_READ  = I2C_DATA_READ_CMD,
    I2C_CMD_LAST_READ     = I2C_LAST_READ_CMD,
} i2c_cmd_t;


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
        REG_I2C_0_CMD = I2C_START_WRITE_CMD;
    } while (!hal_i2c_get_ack());
}

static void hal_i2c_start_read(uint8_t slave_addr){
    REG_I2C_0_TXR = (slave_addr << 1) | 0x01;
    REG_I2C_0_CMD = I2C_START_WRITE_CMD;
    hal_i2c_get_ack();
}

static void hal_i2c_stop(void){
    REG_I2C_0_CMD = I2C_STOP_CMD;
    while (hal_i2c_busy());
}

static void hal_i2c_write_byte(uint8_t byte){
    REG_I2C_0_TXR = byte;
    REG_I2C_0_CMD = I2C_WRITE_BYTE_CMD;
    hal_i2c_get_ack();
}

static uint32_t hal_i2c_read_byte(i2c_cmd_t cmd){
    REG_I2C_0_CMD = cmd;
    hal_i2c_get_ack();
    return REG_I2C_0_RXR;
}


int hal_i2c_init(hal_i2c_port_t port, const hal_i2c_config_t *config){
    if (port >= HAL_I2C_PORT_MAX || !config)
        return -1;

#ifdef CONFIG_STARRYSKY_C2
    hal_gpio_set_function(GPIO_NUM_29, GPIO_FUNCTION_1);
    hal_gpio_set_function(GPIO_NUM_30, GPIO_FUNCTION_1);
#endif

    REG_I2C_0_CTRL = 0;
    REG_I2C_0_PSCR = config->pscr;
    REG_I2C_0_CTRL = 0x80;

    return 0;
}

int hal_i2c_deinit(hal_i2c_port_t port){
    if (port >= HAL_I2C_PORT_MAX)
        return -1;

    REG_I2C_0_CTRL = 0;
    return 0;
}

int hal_i2c_write_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, const uint8_t* data, uint32_t len){
    if (port >= HAL_I2C_PORT_MAX || !data || len == 0)
        return -1;

    hal_i2c_start_write(slave_addr);

    if (reg_addr_len == HAL_I2C_REG_16) {
        hal_i2c_write_byte((reg_addr >> 8) & 0xFF);
    }
    hal_i2c_write_byte(reg_addr & 0xFF);

    for (uint32_t i = 0; i < len; i++) {
        hal_i2c_write_byte(data[i]);
    }

    hal_i2c_stop();
    return 0;
}

int hal_i2c_read_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, uint8_t* data, uint32_t len){
    if (port >= HAL_I2C_PORT_MAX || !data || len == 0)
        return -1;

    hal_i2c_start_write(slave_addr);

    if (reg_addr_len == HAL_I2C_REG_16) {
        hal_i2c_write_byte((reg_addr >> 8) & 0xFF);
    }
    hal_i2c_write_byte(reg_addr & 0xFF);

    hal_i2c_stop();
    hal_i2c_start_read(slave_addr);

    for (uint32_t i = 0; i < len; i++) {
        data[i] = (uint8_t)hal_i2c_read_byte(
            (i == len - 1) ? I2C_CMD_LAST_READ : I2C_CMD_NORMAL_READ);
    }

    hal_i2c_stop();
    return 0;
}
