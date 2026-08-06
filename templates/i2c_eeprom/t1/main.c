#include "main.h"

/* 定义 AT24C64 和 I2C0 测试参数。 */
#define EEPROM_I2C_ADDRESS          0x50u
#define EEPROM_TEST_ADDRESS         0x1FF0u
#define EEPROM_READY_ATTEMPTS       200u
#define I2C_PRESCALER_100KHZ        39u

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
 * 在 AT24C64 指定的十六位内部地址写入一个字节。
 */
static int eeprom_write_byte(uint16_t address, uint8_t value)
{
    /* 通过 SDK I2C HAL 提交一个单字节写事务。 */
    return hal_i2c_write_nbyte(HAL_I2C_PORT_0, EEPROM_I2C_ADDRESS,
                               address, HAL_I2C_REG_16, &value, 1u);
}


/**
 * 重试读取 AT24C64，直到内部写周期结束或达到次数上限。
 */
static int eeprom_wait_and_read(uint16_t address, uint8_t *value)
{
    uint32_t attempt;

    /* 通过随机读事务轮询设备应答并取得目标字节。 */
    for (attempt = 0u; attempt < EEPROM_READY_ATTEMPTS; ++attempt)
    {
        if (hal_i2c_read_nbyte(HAL_I2C_PORT_0, EEPROM_I2C_ADDRESS,
                               address, HAL_I2C_REG_16, value, 1u) == 0)
            return 0;
    }

    /* 报告 EEPROM 在限定次数内没有完成读事务。 */
    return -1;
}


/**
 * 停止测试程序并保留当前现场。
 */
__attribute__((noreturn)) static void eeprom_test_halt(void)
{
    /* 使处理器保持稳定循环，便于观察最后一条串口输出。 */
    for (;;)
        __asm__ volatile("nop");
}


/**
 * 备份、改写、回读并恢复 AT24C64 中的一个测试字节。
 */
int main(void)
{
    const hal_i2c_config_t i2c_config = {
        .pscr = I2C_PRESCALER_100KHZ,
    };
    uint8_t original_value;
    uint8_t test_value;
    uint8_t read_value;
    int test_match;

    /* 初始化串口并说明本次小型非破坏性测试。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1 AT24C64 I2C EEPROM test started.\n");
    hal_sys_putstr("Test address: 0x1FF0, original byte will be restored.\n");

    /* 基于 20 MHz APB 时钟将 I2C0 配置为 100 kHz。 */
    if (hal_i2c_init(HAL_I2C_PORT_0, &i2c_config) != 0)
    {
        hal_sys_putstr("EEPROM FAIL: I2C0 initialization failed.\n");
        eeprom_test_halt();
    }

    /* 读取并打印测试地址中的原始数据。 */
    if (eeprom_wait_and_read(EEPROM_TEST_ADDRESS, &original_value) != 0)
    {
        hal_sys_putstr("EEPROM FAIL: initial read timed out.\n");
        eeprom_test_halt();
    }
    hal_sys_putstr("Original value: ");
    uart_put_hex8(original_value);
    hal_sys_putstr("\n");

    /* 写入原值的反码，确保本次事务实际改变存储内容。 */
    test_value = (uint8_t)~original_value;
    if (eeprom_write_byte(EEPROM_TEST_ADDRESS, test_value) != 0)
    {
        hal_sys_putstr("EEPROM FAIL: test write failed.\n");
        eeprom_test_halt();
    }

    /* 等待写周期结束并记录测试数据的回读结果。 */
    if (eeprom_wait_and_read(EEPROM_TEST_ADDRESS, &read_value) != 0)
    {
        hal_sys_putstr("EEPROM FAIL: test read timed out.\n");
        eeprom_test_halt();
    }
    test_match = read_value == test_value;
    hal_sys_putstr("Test write/read: wrote ");
    uart_put_hex8(test_value);
    hal_sys_putstr(", read ");
    uart_put_hex8(read_value);
    hal_sys_putstr("\n");

    /* 无论测试数据是否匹配都尝试恢复原始数据。 */
    if (eeprom_write_byte(EEPROM_TEST_ADDRESS, original_value) != 0)
    {
        hal_sys_putstr("EEPROM FAIL: original value restore failed.\n");
        eeprom_test_halt();
    }

    /* 等待恢复写周期结束并复核原始数据。 */
    if (eeprom_wait_and_read(EEPROM_TEST_ADDRESS, &read_value) != 0)
    {
        hal_sys_putstr("EEPROM FAIL: restore verification timed out.\n");
        eeprom_test_halt();
    }
    if (read_value != original_value)
    {
        hal_sys_putstr("EEPROM FAIL: restored value mismatch.\n");
        eeprom_test_halt();
    }

    /* 根据测试数据回读结果打印最终结论。 */
    if (test_match != 0)
        hal_sys_putstr("EEPROM PASS: write, read and restore verified.\n");
    else
        hal_sys_putstr("EEPROM FAIL: test value mismatch; original restored.\n");

    /* 关闭 I2C0 并停止测试程序。 */
    (void)hal_i2c_deinit(HAL_I2C_PORT_0);
    eeprom_test_halt();
}
