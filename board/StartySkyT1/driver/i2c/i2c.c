#include <stddef.h>
#include <stdint.h>

#include "hal_i2c.h"
#include "hal_i2c_type.h"
#include "board.h"

#define STARTYSKY_T1_I2C_CTRL_ENABLE          (1u << 7)
#define STARTYSKY_T1_I2C_CMD_START_WRITE      0x90u
#define STARTYSKY_T1_I2C_CMD_WRITE            0x10u
#define STARTYSKY_T1_I2C_CMD_READ             0x20u
#define STARTYSKY_T1_I2C_CMD_STOP             0x40u
#define STARTYSKY_T1_I2C_CMD_STOP_READ        0x68u
#define STARTYSKY_T1_I2C_CMD_CLEAR_STATUS     0x01u
#define STARTYSKY_T1_I2C_STATUS_RX_NACK       (1u << 7)
#define STARTYSKY_T1_I2C_STATUS_BUSY          (1u << 6)
#define STARTYSKY_T1_I2C_STATUS_TRANSFER      (1u << 1)
#define STARTYSKY_T1_I2C_STATUS_COMPLETE      (1u << 0)
#define STARTYSKY_T1_I2C_TIMEOUT              1000000u

/**
 * 等待当前 I2C 命令完成。
 */
static int startysky_t1_i2c_wait_transfer(void)
{
    uint32_t timeout = STARTYSKY_T1_I2C_TIMEOUT;

    /* 等待控制器置位命令完成状态。 */
    while (((REG_I2C_0_SR & STARTYSKY_T1_I2C_STATUS_COMPLETE) == 0u) &&
           (timeout != 0u))
        --timeout;

    if (timeout == 0u)
        return -1;

    /* 等待控制器清除传输进行状态。 */
    timeout = STARTYSKY_T1_I2C_TIMEOUT;
    while (((REG_I2C_0_SR & STARTYSKY_T1_I2C_STATUS_TRANSFER) != 0u) &&
           (timeout != 0u))
        --timeout;

    if (timeout == 0u)
        return -1;

    /* 报告当前命令已经完成。 */
    return 0;
}


/**
 * 等待 I2C 总线进入空闲状态。
 */
static int startysky_t1_i2c_wait_idle(void)
{
    uint32_t timeout = STARTYSKY_T1_I2C_TIMEOUT;

    /* 轮询总线忙状态直到总线被释放。 */
    while (((REG_I2C_0_SR & STARTYSKY_T1_I2C_STATUS_BUSY) != 0u) &&
           (timeout != 0u))
        --timeout;

    /* 根据超时计数返回总线状态。 */
    return timeout == 0u ? -1 : 0;
}


/**
 * 产生停止条件并等待 I2C 总线空闲。
 */
static int startysky_t1_i2c_stop(void)
{
    /* 清除上一条命令状态并请求停止当前事务。 */
    REG_I2C_0_CMD = STARTYSKY_T1_I2C_CMD_CLEAR_STATUS;
    REG_I2C_0_CMD = STARTYSKY_T1_I2C_CMD_STOP;

    /* 等待停止条件执行完成。 */
    return startysky_t1_i2c_wait_idle();
}


/**
 * 使用指定命令发送一个需要应答的 I2C 字节。
 */
static int startysky_t1_i2c_send_byte(uint8_t value, uint8_t command)
{
    /* 清除历史状态并写入本次发送的数据和命令。 */
    REG_I2C_0_CMD = STARTYSKY_T1_I2C_CMD_CLEAR_STATUS;
    REG_I2C_0_TXR = value;
    REG_I2C_0_CMD = command;

    /* 等待传输完成并向上传递超时错误。 */
    if (startysky_t1_i2c_wait_transfer() != 0)
        return -1;

    /* 将从设备未应答状态转换为统一错误。 */
    if ((REG_I2C_0_SR & STARTYSKY_T1_I2C_STATUS_RX_NACK) != 0u)
        return -1;

    /* 报告发送字节已经被从设备应答。 */
    return 0;
}


/**
 * 产生起始条件并发送七位从设备地址和方向位。
 */
static int startysky_t1_i2c_start(uint8_t slave_addr, uint8_t read_direction)
{
    uint8_t address_byte;

    /* 将七位地址和读写方向组合为总线地址字节。 */
    address_byte = (uint8_t)((slave_addr << 1) |
                             (read_direction != 0u ? 1u : 0u));

    /* 发送带起始条件的地址字节。 */
    return startysky_t1_i2c_send_byte(
        address_byte, STARTYSKY_T1_I2C_CMD_START_WRITE);
}


/**
 * 从当前 I2C 事务接收一个字节。
 */
static int startysky_t1_i2c_receive_byte(uint8_t command, uint8_t *value)
{
    /* 清除历史状态并启动本次读取命令。 */
    REG_I2C_0_CMD = STARTYSKY_T1_I2C_CMD_CLEAR_STATUS;
    REG_I2C_0_CMD = command;

    /* 等待读取完成并向上传递超时错误。 */
    if (startysky_t1_i2c_wait_transfer() != 0)
        return -1;

    /* 返回接收寄存器中的低八位数据。 */
    *value = (uint8_t)REG_I2C_0_RXR;
    return 0;
}


/**
 * 在当前事务中发送八位或十六位设备寄存器地址。
 */
static int startysky_t1_i2c_send_register_address(uint16_t reg_addr,
                                                   hal_i2c_reg_addr_len_t reg_addr_len)
{
    /* 对十六位地址先发送高字节。 */
    if (reg_addr_len == HAL_I2C_REG_16)
    {
        if (startysky_t1_i2c_send_byte((uint8_t)(reg_addr >> 8),
                                       STARTYSKY_T1_I2C_CMD_WRITE) != 0)
            return -1;
    }
    else if (reg_addr_len != HAL_I2C_REG_8)
    {
        return -1;
    }

    /* 发送设备寄存器地址的低字节。 */
    return startysky_t1_i2c_send_byte(
        (uint8_t)reg_addr, STARTYSKY_T1_I2C_CMD_WRITE);
}


/**
 * 初始化 StartySky T1 I2C0 控制器。
 */
int hal_i2c_init(hal_i2c_port_t port, const hal_i2c_config_t *config)
{
    /* 拒绝无效端口或空配置。 */
    if ((port != HAL_I2C_PORT_0) || (config == NULL))
        return -1;

    /* 关闭控制器并写入调用方提供的时钟预分频值。 */
    REG_I2C_0_CTRL = 0u;
    REG_I2C_0_PSCR = config->pscr;

    /* 使能控制器并清除复位后残留的完成状态。 */
    REG_I2C_0_CTRL = STARTYSKY_T1_I2C_CTRL_ENABLE;
    REG_I2C_0_CMD = STARTYSKY_T1_I2C_CMD_CLEAR_STATUS;
    return 0;
}


/**
 * 关闭 StartySky T1 I2C0 控制器。
 */
int hal_i2c_deinit(hal_i2c_port_t port)
{
    /* 拒绝 StartySky T1 不支持的 I2C 端口编号。 */
    if (port != HAL_I2C_PORT_0)
        return -1;

    /* 停止当前事务并关闭控制器。 */
    (void)startysky_t1_i2c_stop();
    REG_I2C_0_CTRL = 0u;
    return 0;
}


/**
 * 从指定设备寄存器地址开始写入连续字节。
 */
int hal_i2c_write_nbyte(hal_i2c_port_t port,
                        uint8_t slave_addr,
                        uint16_t reg_addr,
                        hal_i2c_reg_addr_len_t reg_addr_len,
                        const uint8_t *data,
                        uint32_t len)
{
    /* 检查端口、七位地址和数据缓冲区参数。 */
    if ((port != HAL_I2C_PORT_0) || (slave_addr > 0x7Fu) ||
        ((len != 0u) && (data == NULL)))
        return -1;

    /* 以写方向访问从设备并发送设备寄存器地址。 */
    if ((startysky_t1_i2c_start(slave_addr, 0u) != 0) ||
        (startysky_t1_i2c_send_register_address(reg_addr, reg_addr_len) != 0))
    {
        (void)startysky_t1_i2c_stop();
        return -1;
    }

    /* 依次发送调用方提供的全部数据字节。 */
    for (uint32_t index = 0u; index < len; ++index)
    {
        if (startysky_t1_i2c_send_byte(
                data[index], STARTYSKY_T1_I2C_CMD_WRITE) != 0)
        {
            (void)startysky_t1_i2c_stop();
            return -1;
        }
    }

    /* 产生停止条件并返回总线释放结果。 */
    return startysky_t1_i2c_stop();
}


/**
 * 从指定设备寄存器地址开始读取连续字节。
 */
int hal_i2c_read_nbyte(hal_i2c_port_t port,
                       uint8_t slave_addr,
                       uint16_t reg_addr,
                       hal_i2c_reg_addr_len_t reg_addr_len,
                       uint8_t *data,
                       uint32_t len)
{
    /* 检查端口、七位地址、缓冲区和读取长度。 */
    if ((port != HAL_I2C_PORT_0) || (slave_addr > 0x7Fu) ||
        (data == NULL) || (len == 0u))
        return -1;

    /* 以写方向设置设备寄存器地址。 */
    if ((startysky_t1_i2c_start(slave_addr, 0u) != 0) ||
        (startysky_t1_i2c_send_register_address(reg_addr, reg_addr_len) != 0))
    {
        (void)startysky_t1_i2c_stop();
        return -1;
    }

    /* 通过重复起始条件切换到读取方向。 */
    if (startysky_t1_i2c_start(slave_addr, 1u) != 0)
    {
        (void)startysky_t1_i2c_stop();
        return -1;
    }

    /* 读取全部字节，并在最后一个字节同时产生停止条件。 */
    for (uint32_t index = 0u; index < len; ++index)
    {
        uint8_t command = index == (len - 1u)
                              ? STARTYSKY_T1_I2C_CMD_STOP_READ
                              : STARTYSKY_T1_I2C_CMD_READ;

        if (startysky_t1_i2c_receive_byte(command, &data[index]) != 0)
        {
            (void)startysky_t1_i2c_stop();
            return -1;
        }
    }

    /* 最后一个读取命令已经产生停止条件，此处只确认总线空闲。 */
    return startysky_t1_i2c_wait_idle();
}
