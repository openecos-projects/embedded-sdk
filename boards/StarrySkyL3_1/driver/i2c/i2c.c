#include "hal_i2c.h"
#include "board.h"
#include "hal_gpio.h"
#include <stdio.h>

// I2C状态寄存器位定义
#define I2C_STATUS_RXACK     ((uint8_t)0x80) // (1 << 7)
#define I2C_STATUS_BUSY      ((uint8_t)0x40) // (1 << 6)
#define I2C_STATUS_AL        ((uint8_t)0x20) // (1 << 5)
#define I2C_STATUS_TIP       ((uint8_t)0x02) // (1 << 1)
#define I2C_STATUS_IF        ((uint8_t)0x01) // (1 << 0)

// i2c 命令枚举
typedef enum {
    I2C_START = ((uint8_t)0x80),
    I2C_STOP = ((uint8_t)0x40),
    I2C_READ = ((uint8_t)0x20),
    I2C_WRITE = ((uint8_t)0x10),
    I2C_START_READ = ((uint8_t)0xA0),
    I2C_START_WRITE = ((uint8_t)0x90),
    I2C_STOP_READ = ((uint8_t)0x60),
    I2C_STOP_WRITE = ((uint8_t)0x50),
} hal_i2c_cmd_t;

int hal_i2c_init(hal_i2c_port_t port, const hal_i2c_config_t *config){
    if (port != HAL_I2C_PORT_0 || !config) return -1;
    
    // 初始化I2C对应的GPIO复用功能 (L3_1为GPIO_29, GPIO_30)
    gpio_hal_set_fcfg(0, GPIO_NUM_29, 1);
    gpio_hal_set_fcfg(0, GPIO_NUM_30, 1);
    gpio_hal_set_mux(0, GPIO_NUM_29, 0);
    gpio_hal_set_mux(0, GPIO_NUM_30, 0);

    REG_I2C_0_CTRL = (uint8_t)0;
    REG_I2C_0_PSCR = (uint8_t)config->pscr;
    REG_I2C_0_CTRL = (uint8_t)0b10000000;
    
    return 0;
}

int hal_i2c_deinit(hal_i2c_port_t port) {
    if (port != HAL_I2C_PORT_0) return -1;
    REG_I2C_0_CTRL = (uint8_t)0;
    return 0;
}

static uint32_t i2c_get_ack(){
    while((REG_I2C_0_SR & I2C_STATUS_TIP) == 0);
    while((REG_I2C_0_SR & I2C_STATUS_TIP) != 0);
    return !(REG_I2C_0_SR & I2C_STATUS_RXACK);
}

static uint32_t i2c_busy(){
    return ((REG_I2C_0_SR & I2C_STATUS_BUSY) == I2C_STATUS_BUSY);
}

static void i2c_start_write(uint8_t slave_addr){
    do{
        REG_I2C_0_TXR = slave_addr << 1;
        REG_I2C_0_CMD = I2C_START_WRITE;
    }while(!i2c_get_ack());
}

static void i2c_start_read(uint8_t slave_addr){
    REG_I2C_0_TXR = (slave_addr << 1) | 0x1;
    REG_I2C_0_CMD = I2C_START_WRITE;
    if(!i2c_get_ack()){
        printf("I2C: start read no ack\n");
    }
}

static void i2c_stop(){
    REG_I2C_0_CMD = I2C_STOP;
    while(i2c_busy());
}

static void i2c_write_byte(uint8_t byte){
    REG_I2C_0_TXR = byte;
    REG_I2C_0_CMD = I2C_WRITE;
    if(!i2c_get_ack()){
        printf("I2C: write byte no ack\n");
    }
}

static uint32_t i2c_read_byte(hal_i2c_cmd_t cmd){
    REG_I2C_0_CMD = cmd;
    if(!i2c_get_ack()){
        printf("I2C: read byte no ack\n");
    }
    return REG_I2C_0_RXR;
}

int hal_i2c_write_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, const uint8_t* data, uint32_t len){
    if (port != HAL_I2C_PORT_0) return -1;
    
    i2c_start_write(slave_addr);
    if(reg_addr_len == HAL_I2C_REG_8){
        i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    }else{
        i2c_write_byte((uint8_t)((reg_addr >> 8) & 0xFF));
        i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    }
    if (data) {
        for(uint32_t i = 0; i < len; i++){
            i2c_write_byte(data[i]);
        }
    }
    i2c_stop();
    return 0;
}

int hal_i2c_read_nbyte(hal_i2c_port_t port, uint8_t slave_addr, uint16_t reg_addr, hal_i2c_reg_addr_len_t reg_addr_len, uint8_t* data, uint32_t len){
    if (port != HAL_I2C_PORT_0) return -1;
    
    i2c_start_write(slave_addr);
    if(reg_addr_len == HAL_I2C_REG_8){
        i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    }else{
        i2c_write_byte((uint8_t)((reg_addr >> 8) & 0xFF));
        i2c_write_byte((uint8_t)(reg_addr & 0xFF));
    }
    i2c_stop();

    i2c_start_read(slave_addr);
    if (data) {
        for(uint32_t i = 0; i < len; i++){
            data[i] = (uint8_t)i2c_read_byte(i == len - 1 ? I2C_STOP_READ : I2C_READ);
        }
    }
    i2c_stop();
    return 0;
}
