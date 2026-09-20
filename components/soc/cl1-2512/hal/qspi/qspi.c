#include "ecos/hal/qspi.h"
#include "cl1_2512_soc.h"

#include <stddef.h>

/* CL1-2512 QSPI0 是 DesignWare SSI 形态的控制器。该 Target 只支持
 * 命令/地址/dummy/数据阶段的事务（QSPI 协议族），不支持裸数据帧推送
 * （SPI 形式）：write_8/16/32 及其 _cs/批量变体一律返回
 * ECOS_ERR_UNSUPPORTED。 */

#define QSPI_DFS_8_BIT          (7u << 16)
#define QSPI_TMOD_TX_ONLY       (1u << 8)
#define QSPI_TMOD_EEPROM_READ   (3u << 8)
#define QSPI_STATUS_BUSY        (1u << 0)
#define QSPI_STATUS_TX_NOT_FULL (1u << 1)
#define QSPI_STATUS_TX_EMPTY    (1u << 2)
#define QSPI_STATUS_RX_NOT_EMPTY (1u << 3)
#define QSPI_RISR_RX_OVERFLOW   (1u << 3)
#define QSPI_TIMEOUT            1000000u
#define QSPI_SER_CS0            (1u << 0)

/* init 时配置的 SSI 时钟分频值（BAUDR，sck = ssi_clk / div，偶数且 >= 2）。 */
static uint32_t qspi_clock_divider = 10u;

static int qspi_port_is_valid(hal_qspi_port_t port)
{
    return port == HAL_QSPI_PORT_0;
}

/* 禁用控制器、取消片选并清除历史中断状态。 */
static void qspi_reset(void)
{
    REG_QSPI_0_SSIENR = 0u;
    REG_QSPI_0_SER = 0u;
    REG_QSPI_0_IMR = 0u;
    (void)REG_QSPI_0_ICR;
}

/* 复位后写入传输格式并重新使能控制器。 */
static void qspi_configure(uint32_t ctrlr0, uint32_t receive_frames)
{
    qspi_reset();

    REG_QSPI_0_CTRLR0 = ctrlr0;
    REG_QSPI_0_CTRLR1 = receive_frames;
    REG_QSPI_0_SPI_CTRLR0 = 0u;
    REG_QSPI_0_BAUDR = qspi_clock_divider;
    REG_QSPI_0_TXFTLR = 0u;
    REG_QSPI_0_RXFTLR = 0u;
    REG_QSPI_0_SSIENR = 1u;
}

static int qspi_wait_tx_space(void)
{
    uint32_t timeout = QSPI_TIMEOUT;

    while ((REG_QSPI_0_SR & QSPI_STATUS_TX_NOT_FULL) == 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static int qspi_wait_idle(void)
{
    uint32_t timeout = QSPI_TIMEOUT;

    while (timeout != 0u) {
        uint32_t status = REG_QSPI_0_SR;

        if ((status & QSPI_STATUS_BUSY) == 0u &&
            (status & QSPI_STATUS_TX_EMPTY) != 0u)
            return ECOS_OK;
        --timeout;
    }
    return ECOS_ERR_TIMEOUT;
}

static int qspi_wait_rx_data(void)
{
    uint32_t timeout = QSPI_TIMEOUT;

    while (timeout != 0u) {
        if ((REG_QSPI_0_RISR & QSPI_RISR_RX_OVERFLOW) != 0u)
            return ECOS_ERR_IO;
        if ((REG_QSPI_0_SR & QSPI_STATUS_RX_NOT_EMPTY) != 0u)
            return ECOS_OK;
        --timeout;
    }
    return ECOS_ERR_TIMEOUT;
}

/* 命令字节后跟零至四个地址字节，高字节优先写入发送 FIFO。 */
static int qspi_write_header(uint8_t cmd, uint8_t cmd_len,
                             uint32_t addr, uint8_t addr_len)
{
    uint8_t remaining;

    /* 当前轮询实现只接受一个完整命令字节。 */
    if (cmd_len != 8u)
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_QSPI_0_DR = cmd;
    for (remaining = addr_len; remaining != 0u; remaining -= 8u) {
        if (qspi_wait_tx_space() != ECOS_OK)
            return ECOS_ERR_TIMEOUT;
        REG_QSPI_0_DR = (addr >> (remaining - 8u)) & 0xFFu;
    }
    return ECOS_OK;
}

static int qspi_validate_transaction(hal_qspi_port_t port,
                                     uint8_t cmd_len, uint8_t addr_len)
{
    if (!qspi_port_is_valid(port) || cmd_len != 8u ||
        addr_len > 32u || (addr_len % 8u) != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_OK;
}

int hal_qspi_init(hal_qspi_port_t port, const hal_qspi_config_t *config)
{
    if (!qspi_port_is_valid(port) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    /* DesignWare BAUDR 只接受 >= 2 的偶数分频。 */
    if (config->clkdiv < 2u || (config->clkdiv & 1u) != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    qspi_clock_divider = config->clkdiv;
    qspi_configure(QSPI_DFS_8_BIT | QSPI_TMOD_TX_ONLY, 0u);
    return ECOS_OK;
}

int hal_qspi_deinit(hal_qspi_port_t port)
{
    if (!qspi_port_is_valid(port))
        return ECOS_ERR_INVALID_ARGUMENT;

    qspi_reset();
    return ECOS_OK;
}

/* ---- 裸数据帧推送（SPI 形式）：CL1-2512 不支持 ---- */

int hal_qspi_write_8(hal_qspi_port_t port, uint8_t data)
{
    (void)data;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_16(hal_qspi_port_t port, uint16_t data)
{
    (void)data;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32(hal_qspi_port_t port, uint32_t data)
{
    (void)data;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32_repeat(hal_qspi_port_t port, uint32_t data,
                             uint32_t words)
{
    (void)data;
    (void)words;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x2(hal_qspi_port_t port, uint32_t data1, uint32_t data2)
{
    (void)data1;
    (void)data2;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x8(hal_qspi_port_t port, uint32_t data1, uint32_t data2,
                        uint32_t data3, uint32_t data4, uint32_t data5,
                        uint32_t data6, uint32_t data7, uint32_t data8)
{
    (void)data1; (void)data2; (void)data3; (void)data4;
    (void)data5; (void)data6; (void)data7; (void)data8;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x16(hal_qspi_port_t port, uint32_t data1, uint32_t data2,
                         uint32_t data3, uint32_t data4, uint32_t data5,
                         uint32_t data6, uint32_t data7, uint32_t data8,
                         uint32_t data9, uint32_t data10, uint32_t data11,
                         uint32_t data12, uint32_t data13, uint32_t data14,
                         uint32_t data15, uint32_t data16)
{
    (void)data1; (void)data2; (void)data3; (void)data4;
    (void)data5; (void)data6; (void)data7; (void)data8;
    (void)data9; (void)data10; (void)data11; (void)data12;
    (void)data13; (void)data14; (void)data15; (void)data16;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x32(hal_qspi_port_t port, uint32_t data1, uint32_t data2,
                         uint32_t data3, uint32_t data4, uint32_t data5,
                         uint32_t data6, uint32_t data7, uint32_t data8,
                         uint32_t data9, uint32_t data10, uint32_t data11,
                         uint32_t data12, uint32_t data13, uint32_t data14,
                         uint32_t data15, uint32_t data16, uint32_t data17,
                         uint32_t data18, uint32_t data19, uint32_t data20,
                         uint32_t data21, uint32_t data22, uint32_t data23,
                         uint32_t data24, uint32_t data25, uint32_t data26,
                         uint32_t data27, uint32_t data28, uint32_t data29,
                         uint32_t data30, uint32_t data31, uint32_t data32)
{
    (void)data1; (void)data2; (void)data3; (void)data4;
    (void)data5; (void)data6; (void)data7; (void)data8;
    (void)data9; (void)data10; (void)data11; (void)data12;
    (void)data13; (void)data14; (void)data15; (void)data16;
    (void)data17; (void)data18; (void)data19; (void)data20;
    (void)data21; (void)data22; (void)data23; (void)data24;
    (void)data25; (void)data26; (void)data27; (void)data28;
    (void)data29; (void)data30; (void)data31; (void)data32;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_8_cs(hal_qspi_port_t port, uint8_t data, hal_qspi_cs_t cs)
{
    (void)data;
    (void)cs;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_16_cs(hal_qspi_port_t port, uint16_t data,
                         hal_qspi_cs_t cs)
{
    (void)data;
    (void)cs;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32_cs(hal_qspi_port_t port, uint32_t data,
                         hal_qspi_cs_t cs)
{
    (void)data;
    (void)cs;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x2_cs(hal_qspi_port_t port, uint32_t data1,
                           uint32_t data2, hal_qspi_cs_t cs)
{
    (void)data1;
    (void)data2;
    (void)cs;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x8_cs(hal_qspi_port_t port, uint32_t data1,
                           uint32_t data2, uint32_t data3, uint32_t data4,
                           uint32_t data5, uint32_t data6, uint32_t data7,
                           uint32_t data8, hal_qspi_cs_t cs)
{
    (void)data1; (void)data2; (void)data3; (void)data4;
    (void)data5; (void)data6; (void)data7; (void)data8;
    (void)cs;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x16_cs(hal_qspi_port_t port, uint32_t data1,
                            uint32_t data2, uint32_t data3, uint32_t data4,
                            uint32_t data5, uint32_t data6, uint32_t data7,
                            uint32_t data8, uint32_t data9, uint32_t data10,
                            uint32_t data11, uint32_t data12, uint32_t data13,
                            uint32_t data14, uint32_t data15, uint32_t data16,
                            hal_qspi_cs_t cs)
{
    (void)data1; (void)data2; (void)data3; (void)data4;
    (void)data5; (void)data6; (void)data7; (void)data8;
    (void)data9; (void)data10; (void)data11; (void)data12;
    (void)data13; (void)data14; (void)data15; (void)data16;
    (void)cs;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

int hal_qspi_write_32x32_cs(hal_qspi_port_t port, uint32_t data1,
                            uint32_t data2, uint32_t data3, uint32_t data4,
                            uint32_t data5, uint32_t data6, uint32_t data7,
                            uint32_t data8, uint32_t data9, uint32_t data10,
                            uint32_t data11, uint32_t data12, uint32_t data13,
                            uint32_t data14, uint32_t data15, uint32_t data16,
                            uint32_t data17, uint32_t data18, uint32_t data19,
                            uint32_t data20, uint32_t data21, uint32_t data22,
                            uint32_t data23, uint32_t data24, uint32_t data25,
                            uint32_t data26, uint32_t data27, uint32_t data28,
                            uint32_t data29, uint32_t data30, uint32_t data31,
                            uint32_t data32, hal_qspi_cs_t cs)
{
    (void)data1; (void)data2; (void)data3; (void)data4;
    (void)data5; (void)data6; (void)data7; (void)data8;
    (void)data9; (void)data10; (void)data11; (void)data12;
    (void)data13; (void)data14; (void)data15; (void)data16;
    (void)data17; (void)data18; (void)data19; (void)data20;
    (void)data21; (void)data22; (void)data23; (void)data24;
    (void)data25; (void)data26; (void)data27; (void)data28;
    (void)data29; (void)data30; (void)data31; (void)data32;
    (void)cs;
    return qspi_port_is_valid(port) ? ECOS_ERR_UNSUPPORTED
                                    : ECOS_ERR_INVALID_ARGUMENT;
}

/* ---- 命令/地址事务（QSPI 协议族）---- */

int hal_qspi_send_cmd(hal_qspi_port_t port,
                      uint8_t cmd, uint8_t cmd_len,
                      uint32_t addr, uint8_t addr_len)
{
    int result;

    result = qspi_validate_transaction(port, cmd_len, addr_len);
    if (result != ECOS_OK)
        return result;

    qspi_configure(QSPI_DFS_8_BIT | QSPI_TMOD_TX_ONLY, 0u);
    result = qspi_write_header(cmd, cmd_len, addr, addr_len);
    if (result != ECOS_OK)
        return result;

    REG_QSPI_0_SER = QSPI_SER_CS0;
    result = qspi_wait_idle();
    REG_QSPI_0_SER = 0u;
    return result;
}

int hal_qspi_write(hal_qspi_port_t port,
                   uint8_t cmd, uint8_t cmd_len,
                   uint32_t addr, uint8_t addr_len,
                   const uint8_t *tx_buf, uint16_t tx_len)
{
    uint16_t index;
    int result;

    result = qspi_validate_transaction(port, cmd_len, addr_len);
    if (result != ECOS_OK)
        return result;
    if (tx_len != 0u && tx_buf == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (tx_len == 0u)
        return hal_qspi_send_cmd(port, cmd, cmd_len, addr, addr_len);

    qspi_configure(QSPI_DFS_8_BIT | QSPI_TMOD_TX_ONLY, 0u);
    result = qspi_write_header(cmd, cmd_len, addr, addr_len);
    if (result != ECOS_OK)
        return result;

    REG_QSPI_0_SER = QSPI_SER_CS0;
    for (index = 0u; index < tx_len; ++index) {
        result = qspi_wait_tx_space();
        if (result != ECOS_OK) {
            REG_QSPI_0_SER = 0u;
            return result;
        }
        REG_QSPI_0_DR = tx_buf[index];
    }

    result = qspi_wait_idle();
    REG_QSPI_0_SER = 0u;
    return result;
}

int hal_qspi_read(hal_qspi_port_t port,
                  uint8_t cmd, uint8_t cmd_len,
                  uint32_t addr, uint8_t addr_len,
                  uint8_t dummy_cycles,
                  uint8_t *rx_buf, uint16_t rx_len)
{
    uint16_t index;
    uint8_t cycles;
    int result;

    result = qspi_validate_transaction(port, cmd_len, addr_len);
    if (result != ECOS_OK)
        return result;
    if (rx_buf == NULL || rx_len == 0u || (dummy_cycles % 8u) != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    qspi_configure(QSPI_DFS_8_BIT | QSPI_TMOD_EEPROM_READ,
                   (uint32_t)rx_len - 1u);
    result = qspi_write_header(cmd, cmd_len, addr, addr_len);
    if (result != ECOS_OK)
        return result;

    /* 用零字节产生调用方要求的完整 dummy 字节。 */
    for (cycles = dummy_cycles; cycles != 0u; cycles -= 8u) {
        result = qspi_wait_tx_space();
        if (result != ECOS_OK)
            return result;
        REG_QSPI_0_DR = 0u;
    }

    REG_QSPI_0_SER = QSPI_SER_CS0;
    for (index = 0u; index < rx_len; ++index) {
        result = qspi_wait_rx_data();
        if (result != ECOS_OK) {
            REG_QSPI_0_SER = 0u;
            return result;
        }
        rx_buf[index] = (uint8_t)REG_QSPI_0_DR;
    }

    result = qspi_wait_idle();
    REG_QSPI_0_SER = 0u;
    return result;
}
