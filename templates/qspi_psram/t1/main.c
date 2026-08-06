#include "main.h"

/* 定义 QSPI0 传输格式和状态位。 */
#define QSPI_CTRLR0_DFS_32_BIT           (31u << 16)
#define QSPI_CTRLR0_TMOD_TX_ONLY         (1u << 8)
#define QSPI_CTRLR0_TMOD_EEPROM_READ     (3u << 8)
#define QSPI_CTRLR0_SPI_FRF_QUAD         (2u << 21)
#define QSPI_SPI_CTRLR0_TRANS_QPI        (2u << 0)
#define QSPI_SPI_CTRLR0_ADDR_24_BIT      (6u << 2)
#define QSPI_SPI_CTRLR0_INST_8_BIT       (2u << 8)
#define QSPI_SPI_CTRLR0_WAIT_CYCLES(x)   ((uint32_t)(x) << 11)
#define QSPI_STATUS_BUSY                 (1u << 0)
#define QSPI_STATUS_TX_EMPTY             (1u << 2)
#define QSPI_STATUS_RX_NOT_EMPTY         (1u << 3)
#define QSPI_RX_OVERFLOW                 (1u << 3)
#define QSPI_SLAVE_0                     (1u << 0)

/* 定义 ESP_PSRAM64H 和轻量测试参数。 */
#define PSRAM_CMD_ENTER_QPI              0x35u
#define PSRAM_CMD_QUAD_WRITE             0x38u
#define PSRAM_CMD_QUAD_READ              0xEBu
#define PSRAM_READ_DUMMY_CYCLES          6u
#define PSRAM_TEST_ADDRESS               0x00001000u
#define QSPI_BAUD_DIVIDER                10u
#define QSPI_TRANSFER_TIMEOUT            1000000u

/**
 * 通过系统串口打印三十二位十六进制数值。
 */
static void uart_put_hex32(uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    int shift;

    /* 依次打印十六进制前缀和八个数字。 */
    hal_sys_putstr("0x");
    for (shift = 28; shift >= 0; shift -= 4)
        hal_sys_putchar(digits[(value >> shift) & 0x0Fu]);
}


/**
 * 禁用 QSPI0 并清除片选和历史中断状态。
 */
static void qspi_reset_controller(void)
{
    /* 禁用控制器并取消全部片选。 */
    REG_QSPI_0_SSIENR = 0u;
    REG_QSPI_0_SER = 0u;

    /* 屏蔽控制器中断并清除历史错误。 */
    REG_QSPI_0_IMR = 0u;
    (void)REG_QSPI_0_ICR;
}


/**
 * 配置一次 QPI 数据传输并重新使能 QSPI0。
 */
static void qspi_configure_transfer(uint32_t ctrlr0,
                                    uint32_t ctrlr1,
                                    uint32_t spi_ctrlr0)
{
    /* 修改传输格式前复位控制器和 FIFO。 */
    qspi_reset_controller();

    /* 写入数据帧、接收帧数量和增强 QPI 配置。 */
    REG_QSPI_0_CTRLR0 = ctrlr0;
    REG_QSPI_0_CTRLR1 = ctrlr1;
    REG_QSPI_0_SPI_CTRLR0 = spi_ctrlr0;

    /* 完成配置后重新使能控制器。 */
    REG_QSPI_0_SSIENR = 1u;
}


/**
 * 等待 QSPI0 完成发送并进入空闲状态。
 */
static int qspi_wait_idle(void)
{
    uint32_t timeout = QSPI_TRANSFER_TIMEOUT;

    /* 同时等待忙状态清零和发送 FIFO 变空。 */
    while (timeout != 0u)
    {
        uint32_t status = REG_QSPI_0_SR;

        if (((status & QSPI_STATUS_BUSY) == 0u) &&
            ((status & QSPI_STATUS_TX_EMPTY) != 0u))
            return 0;

        --timeout;
    }

    /* 报告控制器未在限定时间内进入空闲状态。 */
    return -1;
}


/**
 * 等待 QSPI0 接收 FIFO 中出现一个三十二位数据帧。
 */
static int qspi_wait_rx_word(void)
{
    uint32_t timeout = QSPI_TRANSFER_TIMEOUT;

    /* 轮询接收 FIFO，并同步检查接收溢出状态。 */
    while (timeout != 0u)
    {
        if ((REG_QSPI_0_RISR & QSPI_RX_OVERFLOW) != 0u)
            return -1;

        if ((REG_QSPI_0_SR & QSPI_STATUS_RX_NOT_EMPTY) != 0u)
            return 0;

        --timeout;
    }

    /* 报告接收 FIFO 未在限定时间内得到数据。 */
    return -1;
}


/**
 * 将 QSPI0 配置为 ESP_PSRAM64H 的三十二位 QPI 写模式。
 */
static void psram_configure_write(void)
{
    uint32_t ctrlr0 = QSPI_CTRLR0_DFS_32_BIT |
                      QSPI_CTRLR0_TMOD_TX_ONLY |
                      QSPI_CTRLR0_SPI_FRF_QUAD;
    uint32_t spi_ctrlr0 = QSPI_SPI_CTRLR0_TRANS_QPI |
                          QSPI_SPI_CTRLR0_ADDR_24_BIT |
                          QSPI_SPI_CTRLR0_INST_8_BIT;

    /* 配置 Quad 指令、24 位地址和三十二位写数据帧。 */
    qspi_configure_transfer(ctrlr0, 0u, spi_ctrlr0);
}


/**
 * 将 QSPI0 配置为 ESP_PSRAM64H 的三十二位 QPI 读模式。
 */
static void psram_configure_read(void)
{
    uint32_t ctrlr0 = QSPI_CTRLR0_DFS_32_BIT |
                      QSPI_CTRLR0_TMOD_EEPROM_READ |
                      QSPI_CTRLR0_SPI_FRF_QUAD;
    uint32_t spi_ctrlr0 = QSPI_SPI_CTRLR0_TRANS_QPI |
                          QSPI_SPI_CTRLR0_ADDR_24_BIT |
                          QSPI_SPI_CTRLR0_INST_8_BIT |
                          QSPI_SPI_CTRLR0_WAIT_CYCLES(
                              PSRAM_READ_DUMMY_CYCLES);

    /* 配置一个三十二位接收帧和 6 个 Quad dummy cycle。 */
    qspi_configure_transfer(ctrlr0, 0u, spi_ctrlr0);
}


/**
 * 通过 QPI 写命令向指定 PSRAM 地址写入一个三十二位字。
 */
static int psram_write_word(uint32_t address, uint32_t data)
{
    int result;

    /* 在片选无效时预装写命令、24 位地址和数据帧。 */
    REG_QSPI_0_SER = 0u;
    REG_QSPI_0_DR = PSRAM_CMD_QUAD_WRITE;
    REG_QSPI_0_DR = address & 0x00FFFFFFu;
    REG_QSPI_0_DR = data;

    /* 选择固定连接的 PSRAM0 并等待写事务完成。 */
    REG_QSPI_0_SER = QSPI_SLAVE_0;
    result = qspi_wait_idle();

    /* 写事务结束后释放 PSRAM0 片选。 */
    REG_QSPI_0_SER = 0u;
    return result;
}


/**
 * 通过 QPI 读命令从指定 PSRAM 地址读取一个三十二位字。
 */
static int psram_read_word(uint32_t address, uint32_t *data)
{
    int result;

    /* 在片选无效时预装读命令和 24 位地址。 */
    REG_QSPI_0_SER = 0u;
    REG_QSPI_0_DR = PSRAM_CMD_QUAD_READ;
    REG_QSPI_0_DR = address & 0x00FFFFFFu;

    /* 选择固定连接的 PSRAM0 并等待接收数据。 */
    REG_QSPI_0_SER = QSPI_SLAVE_0;
    result = qspi_wait_rx_word();
    if (result != 0)
    {
        REG_QSPI_0_SER = 0u;
        return -1;
    }

    /* 读取接收数据并等待控制器完成全部串行时钟。 */
    *data = REG_QSPI_0_DR;
    result = qspi_wait_idle();

    /* 读事务结束后释放 PSRAM0 片选。 */
    REG_QSPI_0_SER = 0u;
    return result;
}


/**
 * 恢复指定 PSRAM 地址中的原始数据并回读复核。
 */
static int psram_restore_word(uint32_t original_value)
{
    uint32_t read_value;

    /* 重新配置写模式并恢复测试前备份的数据。 */
    psram_configure_write();
    if (psram_write_word(PSRAM_TEST_ADDRESS, original_value) != 0)
        return -1;

    /* 重新配置读模式并复核恢复后的数据。 */
    psram_configure_read();
    if (psram_read_word(PSRAM_TEST_ADDRESS, &read_value) != 0)
        return -1;

    /* 返回恢复数据的比较结果。 */
    return read_value == original_value ? 0 : -1;
}


/**
 * 停止测试程序并保留当前现场。
 */
__attribute__((noreturn)) static void psram_test_halt(void)
{
    /* 使处理器保持稳定循环，便于观察最后一条串口输出。 */
    for (;;)
        __asm__ volatile("nop");
}


/**
 * 备份、改写、回读并恢复 ESP_PSRAM64H 中的一个测试字。
 */
int main(void)
{
    const hal_qspi_config_t qspi_config = {
        .clkdiv = QSPI_BAUD_DIVIDER,
    };
    uint32_t original_value;
    uint32_t test_value;
    uint32_t read_value;
    int test_match;

    /* 初始化串口并说明本次轻量非破坏性测试。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1 ESP_PSRAM64H QSPI test started.\n");
    hal_sys_putstr("Test address: 0x00001000, original word will be restored.\n");

    /* 初始化 QSPI0 并使用标准 SPI 命令使 PSRAM 进入 QPI 模式。 */
    if ((hal_qspi_init(HAL_QSPI_PORT_0, &qspi_config) != 0) ||
        (hal_qspi_send_cmd(HAL_QSPI_PORT_0,
                           PSRAM_CMD_ENTER_QPI, 8u, 0u, 0u) != 0))
    {
        hal_sys_putstr("PSRAM FAIL: QSPI initialization failed.\n");
        psram_test_halt();
    }

    /* 读取并打印测试地址中的原始数据。 */
    psram_configure_read();
    if (psram_read_word(PSRAM_TEST_ADDRESS, &original_value) != 0)
    {
        hal_sys_putstr("PSRAM FAIL: initial read timed out.\n");
        psram_test_halt();
    }
    hal_sys_putstr("Original value: ");
    uart_put_hex32(original_value);
    hal_sys_putstr("\n");

    /* 写入原值的反码，确保本次事务实际改变存储内容。 */
    test_value = ~original_value;
    psram_configure_write();
    if (psram_write_word(PSRAM_TEST_ADDRESS, test_value) != 0)
    {
        hal_sys_putstr("PSRAM FAIL: test write timed out.\n");
        if (psram_restore_word(original_value) != 0)
            hal_sys_putstr("PSRAM FAIL: original value restore failed.\n");
        psram_test_halt();
    }

    /* 回读测试数据并在异常时尽量恢复原始数据。 */
    psram_configure_read();
    if (psram_read_word(PSRAM_TEST_ADDRESS, &read_value) != 0)
    {
        hal_sys_putstr("PSRAM FAIL: test read timed out.\n");
        if (psram_restore_word(original_value) != 0)
            hal_sys_putstr("PSRAM FAIL: original value restore failed.\n");
        psram_test_halt();
    }
    test_match = read_value == test_value;
    hal_sys_putstr("Test write/read: wrote ");
    uart_put_hex32(test_value);
    hal_sys_putstr(", read ");
    uart_put_hex32(read_value);
    hal_sys_putstr("\n");

    /* 恢复并复核测试地址中的原始数据。 */
    if (psram_restore_word(original_value) != 0)
    {
        hal_sys_putstr("PSRAM FAIL: original value restore failed.\n");
        psram_test_halt();
    }

    /* 根据测试数据回读结果打印最终结论。 */
    if (test_match != 0)
        hal_sys_putstr("PSRAM PASS: write, read and restore verified.\n");
    else
        hal_sys_putstr("PSRAM FAIL: test value mismatch; original restored.\n");

    /* 关闭 QSPI0 并停止测试程序。 */
    (void)hal_qspi_deinit(HAL_QSPI_PORT_0);
    psram_test_halt();
}
