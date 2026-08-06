#include "main.h"

/* 定义 I2C0 地址扫描使用的控制命令和状态位。 */
#define I2C_CMD_START_WRITE          0x90u
#define I2C_CMD_STOP                 0x40u
#define I2C_CMD_CLEAR_STATUS         0x01u
#define I2C_STATUS_RX_NACK           (1u << 7)
#define I2C_STATUS_BUSY              (1u << 6)
#define I2C_STATUS_TRANSFER          (1u << 1)
#define I2C_STATUS_COMPLETE          (1u << 0)

/* 定义扫描范围、超时和 100 kHz 预分频参数。 */
#define I2C_SCAN_FIRST_ADDRESS       0x08u
#define I2C_SCAN_LAST_ADDRESS        0x77u
#define I2C_ONBOARD_EEPROM_ADDRESS   0x50u
#define I2C_TRANSFER_TIMEOUT         1000000u
#define I2C_PRESCALER_100KHZ         39u

/**
 * 通过系统串口打印八位十六进制数值。
 */
static void uart_put_hex8(uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";

    /* 依次打印十六进制前缀和两个数字。 */
    hal_sys_putstr("0x");
    hal_sys_putchar(digits[(value >> 4) & 0x0Fu]);
    hal_sys_putchar(digits[value & 0x0Fu]);
}


/**
 * 等待当前 I2C 地址传输完成。
 */
static int i2c_wait_transfer(void)
{
    uint32_t timeout = I2C_TRANSFER_TIMEOUT;

    /* 等待控制器置位命令完成状态。 */
    while (((REG_I2C_0_SR & I2C_STATUS_COMPLETE) == 0u) &&
           (timeout != 0u))
        --timeout;

    if (timeout == 0u)
        return -1;

    /* 等待控制器清除传输进行状态。 */
    timeout = I2C_TRANSFER_TIMEOUT;
    while (((REG_I2C_0_SR & I2C_STATUS_TRANSFER) != 0u) &&
           (timeout != 0u))
        --timeout;

    /* 根据超时计数返回传输结果。 */
    return timeout == 0u ? -1 : 0;
}


/**
 * 产生停止条件并等待 I2C 总线恢复空闲。
 */
static int i2c_stop(void)
{
    uint32_t timeout = I2C_TRANSFER_TIMEOUT;

    /* 清除历史状态并请求停止当前事务。 */
    REG_I2C_0_CMD = I2C_CMD_CLEAR_STATUS;
    REG_I2C_0_CMD = I2C_CMD_STOP;

    /* 等待控制器释放 I2C 总线。 */
    while (((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u) &&
           (timeout != 0u))
        --timeout;

    /* 根据超时计数返回总线释放结果。 */
    return timeout == 0u ? -1 : 0;
}


/**
 * 仅发送指定七位地址的写方向字节并检查设备应答。
 */
static int i2c_probe_address(uint8_t address)
{
    int found;

    /* 清除历史状态并发送带起始条件的地址字节。 */
    REG_I2C_0_CMD = I2C_CMD_CLEAR_STATUS;
    REG_I2C_0_TXR = (uint32_t)address << 1;
    REG_I2C_0_CMD = I2C_CMD_START_WRITE;

    /* 等待地址传输完成并在超时时尝试释放总线。 */
    if (i2c_wait_transfer() != 0)
    {
        (void)i2c_stop();
        return -1;
    }

    /* 在停止事务前保存从设备的应答结果。 */
    found = (REG_I2C_0_SR & I2C_STATUS_RX_NACK) == 0u;
    if (i2c_stop() != 0)
        return -1;

    /* 返回 1 表示设备应答，返回 0 表示地址未应答。 */
    return found;
}


/**
 * 扫描标准七位 I2C 地址范围并打印所有应答地址。
 */
static int i2c_scan_bus(void)
{
    uint32_t address;
    int device_count = 0;

    /* 依次探测排除保留区域后的标准七位地址。 */
    for (address = I2C_SCAN_FIRST_ADDRESS;
         address <= I2C_SCAN_LAST_ADDRESS; ++address)
    {
        int result = i2c_probe_address((uint8_t)address);

        /* 遇到控制器或总线错误时报告地址并终止扫描。 */
        if (result < 0)
        {
            hal_sys_putstr("I2C scan FAIL at address ");
            uart_put_hex8((uint8_t)address);
            hal_sys_putstr(".\n");
            return -1;
        }

        /* 打印当前返回应答的设备地址。 */
        if (result != 0)
        {
            ++device_count;
            hal_sys_putstr("ACK: ");
            uart_put_hex8((uint8_t)address);
            if (address == I2C_ONBOARD_EEPROM_ADDRESS)
                hal_sys_putstr(" (on-board EEPROM)");
            hal_sys_putstr("\n");
        }
    }

    /* 返回本轮扫描发现的设备数量。 */
    return device_count;
}


/**
 * 停止扫描程序并保留当前现场。
 */
__attribute__((noreturn)) static void i2c_scan_halt(void)
{
    /* 使处理器保持稳定循环，便于观察最后一条串口输出。 */
    for (;;)
        __asm__ volatile("nop");
}


/**
 * 初始化 I2C0 并执行一次无数据地址扫描。
 */
int main(void)
{
    const hal_i2c_config_t i2c_config = {
        .pscr = I2C_PRESCALER_100KHZ,
    };
    int device_count;

    /* 初始化串口并说明扫描不会向从设备写入数据。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1 I2C address scan started.\n");
    hal_sys_putstr("Range: 0x08-0x77, address only, no data write.\n");

    /* 基于 20 MHz APB 时钟将 I2C0 配置为 100 kHz。 */
    if (hal_i2c_init(HAL_I2C_PORT_0, &i2c_config) != 0)
    {
        hal_sys_putstr("I2C scan FAIL: initialization failed.\n");
        i2c_scan_halt();
    }

    /* 执行一次总线扫描并打印最终结果。 */
    device_count = i2c_scan_bus();
    if (device_count < 0)
    {
        (void)hal_i2c_deinit(HAL_I2C_PORT_0);
        i2c_scan_halt();
    }
    if (device_count == 0)
        hal_sys_putstr("I2C scan complete: no devices found.\n");
    else
    {
        hal_sys_putstr("I2C scan PASS: found ");
        uart_put_hex8((uint8_t)device_count);
        hal_sys_putstr(" device(s).\n");
    }

    /* 关闭 I2C0 并停止扫描程序。 */
    (void)hal_i2c_deinit(HAL_I2C_PORT_0);
    i2c_scan_halt();
}
