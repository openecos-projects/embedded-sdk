#include <stddef.h>
#include <stdint.h>

#include "hal_qspi.h"
#include "board.h"

#define STARTYSKY_T1_PICO_QSPI_DFS_8_BIT             (7u << 16)
#define STARTYSKY_T1_PICO_QSPI_DFS_16_BIT            (15u << 16)
#define STARTYSKY_T1_PICO_QSPI_DFS_32_BIT            (31u << 16)
#define STARTYSKY_T1_PICO_QSPI_TMOD_TX_ONLY          (1u << 8)
#define STARTYSKY_T1_PICO_QSPI_TMOD_EEPROM_READ      (3u << 8)
#define STARTYSKY_T1_PICO_QSPI_STATUS_BUSY           (1u << 0)
#define STARTYSKY_T1_PICO_QSPI_STATUS_TX_NOT_FULL    (1u << 1)
#define STARTYSKY_T1_PICO_QSPI_STATUS_TX_EMPTY       (1u << 2)
#define STARTYSKY_T1_PICO_QSPI_STATUS_RX_NOT_EMPTY   (1u << 3)
#define STARTYSKY_T1_PICO_QSPI_RX_OVERFLOW           (1u << 3)
#define STARTYSKY_T1_PICO_QSPI_TIMEOUT               1000000u

/* 保存初始化时配置的 QSPI 时钟分频值。 */
static uint32_t qspi_clock_divider = 10u;

/**
 * 将 SDK 片选编号转换为 DesignWare SSI 片选位。
 */
static uint32_t startysky_t1_pico_qspi_chip_select(hal_qspi_cs_t cs)
{
    /* 将四个 SDK 片选枚举映射到对应的硬件位。 */
    switch (cs)
    {
    case HAL_QSPI_CS_0:
        return 1u << 0;
    case HAL_QSPI_CS_1:
        return 1u << 1;
    case HAL_QSPI_CS_2:
        return 1u << 2;
    case HAL_QSPI_CS_3:
        return 1u << 3;
    default:
        return 0u;
    }
}


/**
 * 禁用 QSPI 控制器并清除片选和历史中断状态。
 */
static void startysky_t1_pico_qspi_reset(void)
{
    /* 禁用控制器并取消全部片选。 */
    REG_QSPI_0_SSIENR = 0u;
    REG_QSPI_0_SER = 0u;

    /* 屏蔽控制器中断并清除历史错误。 */
    REG_QSPI_0_IMR = 0u;
    (void)REG_QSPI_0_ICR;
}


/**
 * 配置 QSPI 数据帧和传输模式。
 */
static void startysky_t1_pico_qspi_configure(uint32_t ctrlr0, uint32_t receive_frames)
{
    /* 修改传输格式前复位控制器和 FIFO。 */
    startysky_t1_pico_qspi_reset();

    /* 写入传输模式、接收帧数量和标准 SPI 配置。 */
    REG_QSPI_0_CTRLR0 = ctrlr0;
    REG_QSPI_0_CTRLR1 = receive_frames;
    REG_QSPI_0_SPI_CTRLR0 = 0u;
    REG_QSPI_0_BAUDR = qspi_clock_divider;
    REG_QSPI_0_TXFTLR = 0u;
    REG_QSPI_0_RXFTLR = 0u;

    /* 重新使能已经配置完成的控制器。 */
    REG_QSPI_0_SSIENR = 1u;
}


/**
 * 等待 QSPI 发送 FIFO 能够接收一个数据帧。
 */
static int startysky_t1_pico_qspi_wait_tx_space(void)
{
    uint32_t timeout = STARTYSKY_T1_PICO_QSPI_TIMEOUT;

    /* 轮询发送 FIFO 非满状态。 */
    while (((REG_QSPI_0_SR & STARTYSKY_T1_PICO_QSPI_STATUS_TX_NOT_FULL) == 0u) &&
           (timeout != 0u))
        --timeout;

    /* 根据超时计数返回 FIFO 状态。 */
    return timeout == 0u ? -1 : 0;
}


/**
 * 等待 QSPI 控制器完成发送并进入空闲状态。
 */
static int startysky_t1_pico_qspi_wait_idle(void)
{
    uint32_t timeout = STARTYSKY_T1_PICO_QSPI_TIMEOUT;

    /* 同时等待忙状态清零和发送 FIFO 变空。 */
    while (timeout != 0u)
    {
        uint32_t status = REG_QSPI_0_SR;

        if (((status & STARTYSKY_T1_PICO_QSPI_STATUS_BUSY) == 0u) &&
            ((status & STARTYSKY_T1_PICO_QSPI_STATUS_TX_EMPTY) != 0u))
            return 0;

        --timeout;
    }

    /* 报告控制器未能在限定时间内进入空闲状态。 */
    return -1;
}


/**
 * 等待 QSPI 接收 FIFO 中出现一个数据帧。
 */
static int startysky_t1_pico_qspi_wait_rx_data(void)
{
    uint32_t timeout = STARTYSKY_T1_PICO_QSPI_TIMEOUT;

    /* 轮询接收 FIFO，并同步检查接收溢出状态。 */
    while (timeout != 0u)
    {
        if ((REG_QSPI_0_RISR & STARTYSKY_T1_PICO_QSPI_RX_OVERFLOW) != 0u)
            return -1;

        if ((REG_QSPI_0_SR & STARTYSKY_T1_PICO_QSPI_STATUS_RX_NOT_EMPTY) != 0u)
            return 0;

        --timeout;
    }

    /* 报告接收 FIFO 未在限定时间内得到数据。 */
    return -1;
}


/**
 * 使用指定片选发送一组同宽数据帧。
 */
static int startysky_t1_pico_qspi_write_frames(const uint32_t *frames,
                                  uint32_t frame_count,
                                  uint32_t frame_format,
                                  hal_qspi_cs_t cs)
{
    uint32_t chip_select;

    /* 检查数据缓冲区、帧数量和片选编号。 */
    chip_select = startysky_t1_pico_qspi_chip_select(cs);
    if ((frames == NULL) || (frame_count == 0u) || (chip_select == 0u))
        return -1;

    /* 配置仅发送模式并在片选无效时预填第一帧。 */
    startysky_t1_pico_qspi_configure(frame_format | STARTYSKY_T1_PICO_QSPI_TMOD_TX_ONLY, 0u);
    REG_QSPI_0_DR = frames[0];
    REG_QSPI_0_SER = chip_select;

    /* 控制器发送前一帧时继续填入剩余数据帧。 */
    for (uint32_t index = 1u; index < frame_count; ++index)
    {
        if (startysky_t1_pico_qspi_wait_tx_space() != 0)
        {
            REG_QSPI_0_SER = 0u;
            return -1;
        }

        REG_QSPI_0_DR = frames[index];
    }

    /* 等待全部串行传输完成并释放片选。 */
    if (startysky_t1_pico_qspi_wait_idle() != 0)
    {
        REG_QSPI_0_SER = 0u;
        return -1;
    }

    REG_QSPI_0_SER = 0u;
    return 0;
}


/**
 * 将命令和地址按高字节优先顺序写入发送 FIFO。
 */
static int startysky_t1_pico_qspi_write_header(uint8_t cmd,
                                  uint8_t cmd_len,
                                  uint32_t addr,
                                  uint8_t addr_len)
{
    /* 当前标准 SPI 实现只接受一个完整命令字节。 */
    if (cmd_len != 8u)
        return -1;

    /* 先写入命令字节。 */
    REG_QSPI_0_DR = cmd;

    /* 按高字节优先顺序写入零至四个地址字节。 */
    for (uint8_t remaining = addr_len; remaining != 0u; remaining -= 8u)
    {
        if (startysky_t1_pico_qspi_wait_tx_space() != 0)
            return -1;

        REG_QSPI_0_DR = (addr >> (remaining - 8u)) & 0xFFu;
    }

    /* 报告命令和地址已经全部写入。 */
    return 0;
}


/**
 * 检查新 QSPI API 的公共参数。
 */
static int startysky_t1_pico_qspi_validate_transaction(hal_qspi_port_t port,
                                          uint8_t cmd_len,
                                          uint8_t addr_len)
{
    /* 检查端口、命令宽度和字节对齐的地址宽度。 */
    if ((port != HAL_QSPI_PORT_0) || (cmd_len != 8u) ||
        (addr_len > 32u) || ((addr_len % 8u) != 0u))
        return -1;

    /* 报告事务参数符合标准 SPI 轮询实现。 */
    return 0;
}


/**
 * 初始化 StartySky T1-Pico QSPI0 控制器。
 */
int hal_qspi_init(hal_qspi_port_t port, const hal_qspi_config_t *config)
{
    /* 拒绝无效端口、空配置和无效分频值。 */
    if ((port != HAL_QSPI_PORT_0) || (config == NULL) ||
        (config->clkdiv < 2u) || ((config->clkdiv & 1u) != 0u))
        return -1;

    /* 保存分频值并配置八位标准 SPI 发送模式。 */
    qspi_clock_divider = config->clkdiv;
    startysky_t1_pico_qspi_configure(STARTYSKY_T1_PICO_QSPI_DFS_8_BIT | STARTYSKY_T1_PICO_QSPI_TMOD_TX_ONLY, 0u);
    return 0;
}


/**
 * 关闭 StartySky T1-Pico QSPI0 控制器。
 */
int hal_qspi_deinit(hal_qspi_port_t port)
{
    /* 拒绝 StartySky T1-Pico 不支持的 QSPI 端口编号。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    /* 释放片选并禁用控制器。 */
    startysky_t1_pico_qspi_reset();
    return 0;
}


/**
 * 使用默认片选发送八位数据。
 */
int hal_qspi_write_8(hal_qspi_port_t port, uint8_t data)
{
    /* 将单个八位数据帧发送到片选零。 */
    return hal_qspi_write_8_cs(port, data, HAL_QSPI_CS_0);
}


/**
 * 使用默认片选发送十六位数据。
 */
int hal_qspi_write_16(hal_qspi_port_t port, uint16_t data)
{
    /* 将单个十六位数据帧发送到片选零。 */
    return hal_qspi_write_16_cs(port, data, HAL_QSPI_CS_0);
}


/**
 * 使用默认片选发送三十二位数据。
 */
int hal_qspi_write_32(hal_qspi_port_t port, uint32_t data)
{
    /* 将单个三十二位数据帧发送到片选零。 */
    return hal_qspi_write_32_cs(port, data, HAL_QSPI_CS_0);
}


/**
 * 使用默认片选重复发送三十二位数据。
 */
int hal_qspi_write_32_repeat(hal_qspi_port_t port,
                             uint32_t data,
                             uint32_t words)
{
    /* 检查端口和重复次数。 */
    if ((port != HAL_QSPI_PORT_0) || (words == 0u))
        return -1;

    /* 配置三十二位仅发送模式并预填第一帧。 */
    startysky_t1_pico_qspi_configure(STARTYSKY_T1_PICO_QSPI_DFS_32_BIT | STARTYSKY_T1_PICO_QSPI_TMOD_TX_ONLY, 0u);
    REG_QSPI_0_DR = data;
    REG_QSPI_0_SER = startysky_t1_pico_qspi_chip_select(HAL_QSPI_CS_0);

    /* 在控制器发送数据时继续填入剩余重复帧。 */
    for (uint32_t index = 1u; index < words; ++index)
    {
        if (startysky_t1_pico_qspi_wait_tx_space() != 0)
        {
            REG_QSPI_0_SER = 0u;
            return -1;
        }

        REG_QSPI_0_DR = data;
    }

    /* 等待发送完成并释放默认片选。 */
    if (startysky_t1_pico_qspi_wait_idle() != 0)
    {
        REG_QSPI_0_SER = 0u;
        return -1;
    }

    REG_QSPI_0_SER = 0u;
    return 0;
}


/**
 * 使用默认片选发送两个三十二位数据。
 */
int hal_qspi_write_32x2(hal_qspi_port_t port,
                        uint32_t data1,
                        uint32_t data2)
{
    /* 转发到带片选参数的双字发送接口。 */
    return hal_qspi_write_32x2_cs(port,
                                  data1,
                                  data2,
                                  HAL_QSPI_CS_0);
}


/**
 * 使用默认片选发送八个三十二位数据。
 */
int hal_qspi_write_32x8(hal_qspi_port_t port,
                        uint32_t data1,
                        uint32_t data2,
                        uint32_t data3,
                        uint32_t data4,
                        uint32_t data5,
                        uint32_t data6,
                        uint32_t data7,
                        uint32_t data8)
{
    /* 转发到带片选参数的八字发送接口。 */
    return hal_qspi_write_32x8_cs(port,
                                  data1, data2, data3, data4,
                                  data5, data6, data7, data8,
                                  HAL_QSPI_CS_0);
}


/**
 * 使用默认片选发送十六个三十二位数据。
 */
int hal_qspi_write_32x16(hal_qspi_port_t port,
                         uint32_t data1, uint32_t data2,
                         uint32_t data3, uint32_t data4,
                         uint32_t data5, uint32_t data6,
                         uint32_t data7, uint32_t data8,
                         uint32_t data9, uint32_t data10,
                         uint32_t data11, uint32_t data12,
                         uint32_t data13, uint32_t data14,
                         uint32_t data15, uint32_t data16)
{
    /* 转发到带片选参数的十六字发送接口。 */
    return hal_qspi_write_32x16_cs(port,
                                   data1, data2, data3, data4,
                                   data5, data6, data7, data8,
                                   data9, data10, data11, data12,
                                   data13, data14, data15, data16,
                                   HAL_QSPI_CS_0);
}


/**
 * 使用默认片选发送三十二个三十二位数据。
 */
int hal_qspi_write_32x32(hal_qspi_port_t port,
                         uint32_t data1, uint32_t data2,
                         uint32_t data3, uint32_t data4,
                         uint32_t data5, uint32_t data6,
                         uint32_t data7, uint32_t data8,
                         uint32_t data9, uint32_t data10,
                         uint32_t data11, uint32_t data12,
                         uint32_t data13, uint32_t data14,
                         uint32_t data15, uint32_t data16,
                         uint32_t data17, uint32_t data18,
                         uint32_t data19, uint32_t data20,
                         uint32_t data21, uint32_t data22,
                         uint32_t data23, uint32_t data24,
                         uint32_t data25, uint32_t data26,
                         uint32_t data27, uint32_t data28,
                         uint32_t data29, uint32_t data30,
                         uint32_t data31, uint32_t data32)
{
    /* 转发到带片选参数的三十二字发送接口。 */
    return hal_qspi_write_32x32_cs(port,
                                   data1, data2, data3, data4,
                                   data5, data6, data7, data8,
                                   data9, data10, data11, data12,
                                   data13, data14, data15, data16,
                                   data17, data18, data19, data20,
                                   data21, data22, data23, data24,
                                   data25, data26, data27, data28,
                                   data29, data30, data31, data32,
                                   HAL_QSPI_CS_0);
}


/**
 * 使用指定片选发送八位数据。
 */
int hal_qspi_write_8_cs(hal_qspi_port_t port,
                        uint8_t data,
                        hal_qspi_cs_t cs)
{
    uint32_t frame = data;

    /* 检查端口并发送单个八位数据帧。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    return startysky_t1_pico_qspi_write_frames(&frame, 1u, STARTYSKY_T1_PICO_QSPI_DFS_8_BIT, cs);
}


/**
 * 使用指定片选发送十六位数据。
 */
int hal_qspi_write_16_cs(hal_qspi_port_t port,
                         uint16_t data,
                         hal_qspi_cs_t cs)
{
    uint32_t frame = data;

    /* 检查端口并发送单个十六位数据帧。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    return startysky_t1_pico_qspi_write_frames(&frame, 1u, STARTYSKY_T1_PICO_QSPI_DFS_16_BIT, cs);
}


/**
 * 使用指定片选发送三十二位数据。
 */
int hal_qspi_write_32_cs(hal_qspi_port_t port,
                         uint32_t data,
                         hal_qspi_cs_t cs)
{
    /* 检查端口并发送单个三十二位数据帧。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    return startysky_t1_pico_qspi_write_frames(&data, 1u, STARTYSKY_T1_PICO_QSPI_DFS_32_BIT, cs);
}


/**
 * 使用指定片选发送两个三十二位数据。
 */
int hal_qspi_write_32x2_cs(hal_qspi_port_t port,
                           uint32_t data1,
                           uint32_t data2,
                           hal_qspi_cs_t cs)
{
    uint32_t frames[2] =
    {
        data1, data2
    };

    /* 检查端口并发送两个三十二位数据帧。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    return startysky_t1_pico_qspi_write_frames(frames, 2u, STARTYSKY_T1_PICO_QSPI_DFS_32_BIT, cs);
}


/**
 * 使用指定片选发送八个三十二位数据。
 */
int hal_qspi_write_32x8_cs(hal_qspi_port_t port,
                           uint32_t data1, uint32_t data2,
                           uint32_t data3, uint32_t data4,
                           uint32_t data5, uint32_t data6,
                           uint32_t data7, uint32_t data8,
                           hal_qspi_cs_t cs)
{
    uint32_t frames[8] =
    {
        data1, data2, data3, data4,
        data5, data6, data7, data8
    };

    /* 检查端口并发送八个三十二位数据帧。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    return startysky_t1_pico_qspi_write_frames(frames, 8u, STARTYSKY_T1_PICO_QSPI_DFS_32_BIT, cs);
}


/**
 * 使用指定片选发送十六个三十二位数据。
 */
int hal_qspi_write_32x16_cs(hal_qspi_port_t port,
                            uint32_t data1, uint32_t data2,
                            uint32_t data3, uint32_t data4,
                            uint32_t data5, uint32_t data6,
                            uint32_t data7, uint32_t data8,
                            uint32_t data9, uint32_t data10,
                            uint32_t data11, uint32_t data12,
                            uint32_t data13, uint32_t data14,
                            uint32_t data15, uint32_t data16,
                            hal_qspi_cs_t cs)
{
    uint32_t frames[16] =
    {
        data1, data2, data3, data4,
        data5, data6, data7, data8,
        data9, data10, data11, data12,
        data13, data14, data15, data16
    };

    /* 检查端口并发送十六个三十二位数据帧。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    return startysky_t1_pico_qspi_write_frames(frames, 16u, STARTYSKY_T1_PICO_QSPI_DFS_32_BIT, cs);
}


/**
 * 使用指定片选发送三十二个三十二位数据。
 */
int hal_qspi_write_32x32_cs(hal_qspi_port_t port,
                            uint32_t data1, uint32_t data2,
                            uint32_t data3, uint32_t data4,
                            uint32_t data5, uint32_t data6,
                            uint32_t data7, uint32_t data8,
                            uint32_t data9, uint32_t data10,
                            uint32_t data11, uint32_t data12,
                            uint32_t data13, uint32_t data14,
                            uint32_t data15, uint32_t data16,
                            uint32_t data17, uint32_t data18,
                            uint32_t data19, uint32_t data20,
                            uint32_t data21, uint32_t data22,
                            uint32_t data23, uint32_t data24,
                            uint32_t data25, uint32_t data26,
                            uint32_t data27, uint32_t data28,
                            uint32_t data29, uint32_t data30,
                            uint32_t data31, uint32_t data32,
                            hal_qspi_cs_t cs)
{
    uint32_t frames[32] =
    {
        data1, data2, data3, data4,
        data5, data6, data7, data8,
        data9, data10, data11, data12,
        data13, data14, data15, data16,
        data17, data18, data19, data20,
        data21, data22, data23, data24,
        data25, data26, data27, data28,
        data29, data30, data31, data32
    };

    /* 检查端口并发送三十二个三十二位数据帧。 */
    if (port != HAL_QSPI_PORT_0)
        return -1;

    return startysky_t1_pico_qspi_write_frames(frames, 32u, STARTYSKY_T1_PICO_QSPI_DFS_32_BIT, cs);
}


/**
 * 发送不包含数据阶段的标准 SPI 命令和地址。
 */
int hal_qspi_send_cmd(hal_qspi_port_t port,
                      uint8_t cmd,
                      uint8_t cmd_len,
                      uint32_t addr,
                      uint8_t addr_len)
{
    /* 检查标准 SPI 命令和地址参数。 */
    if (startysky_t1_pico_qspi_validate_transaction(port, cmd_len, addr_len) != 0)
        return -1;

    /* 配置八位仅发送模式并写入命令和地址。 */
    startysky_t1_pico_qspi_configure(STARTYSKY_T1_PICO_QSPI_DFS_8_BIT | STARTYSKY_T1_PICO_QSPI_TMOD_TX_ONLY, 0u);
    if (startysky_t1_pico_qspi_write_header(cmd, cmd_len, addr, addr_len) != 0)
        return -1;

    /* 选择片选零并等待命令发送完成。 */
    REG_QSPI_0_SER = startysky_t1_pico_qspi_chip_select(HAL_QSPI_CS_0);
    if (startysky_t1_pico_qspi_wait_idle() != 0)
    {
        REG_QSPI_0_SER = 0u;
        return -1;
    }

    REG_QSPI_0_SER = 0u;
    return 0;
}


/**
 * 使用标准 SPI 命令和地址写入连续字节。
 */
int hal_qspi_write(hal_qspi_port_t port,
                   uint8_t cmd,
                   uint8_t cmd_len,
                   uint32_t addr,
                   uint8_t addr_len,
                   const uint8_t *tx_buf,
                   uint16_t tx_len)
{
    /* 检查事务参数和发送缓冲区。 */
    if ((startysky_t1_pico_qspi_validate_transaction(port, cmd_len, addr_len) != 0) ||
        ((tx_len != 0u) && (tx_buf == NULL)))
        return -1;

    /* 无数据事务直接使用命令发送接口。 */
    if (tx_len == 0u)
        return hal_qspi_send_cmd(port, cmd, cmd_len, addr, addr_len);

    /* 配置八位仅发送模式并预填命令和地址。 */
    startysky_t1_pico_qspi_configure(STARTYSKY_T1_PICO_QSPI_DFS_8_BIT | STARTYSKY_T1_PICO_QSPI_TMOD_TX_ONLY, 0u);
    if (startysky_t1_pico_qspi_write_header(cmd, cmd_len, addr, addr_len) != 0)
        return -1;

    REG_QSPI_0_SER = startysky_t1_pico_qspi_chip_select(HAL_QSPI_CS_0);

    /* 在控制器发送头部时继续写入全部数据字节。 */
    for (uint16_t index = 0u; index < tx_len; ++index)
    {
        if (startysky_t1_pico_qspi_wait_tx_space() != 0)
        {
            REG_QSPI_0_SER = 0u;
            return -1;
        }

        REG_QSPI_0_DR = tx_buf[index];
    }

    /* 等待事务完成并释放片选零。 */
    if (startysky_t1_pico_qspi_wait_idle() != 0)
    {
        REG_QSPI_0_SER = 0u;
        return -1;
    }

    REG_QSPI_0_SER = 0u;
    return 0;
}


/**
 * 使用标准 SPI 命令和地址读取连续字节。
 */
int hal_qspi_read(hal_qspi_port_t port,
                  uint8_t cmd,
                  uint8_t cmd_len,
                  uint32_t addr,
                  uint8_t addr_len,
                  uint8_t dummy_cycles,
                  uint8_t *rx_buf,
                  uint16_t rx_len)
{
    /* 检查事务参数、接收缓冲区和字节对齐的 dummy 周期。 */
    if ((startysky_t1_pico_qspi_validate_transaction(port, cmd_len, addr_len) != 0) ||
        (rx_buf == NULL) || (rx_len == 0u) ||
        ((dummy_cycles % 8u) != 0u))
        return -1;

    /* 配置八位 EEPROM 读取模式和目标接收帧数。 */
    startysky_t1_pico_qspi_configure(STARTYSKY_T1_PICO_QSPI_DFS_8_BIT | STARTYSKY_T1_PICO_QSPI_TMOD_EEPROM_READ,
                       (uint32_t)rx_len - 1u);
    if (startysky_t1_pico_qspi_write_header(cmd, cmd_len, addr, addr_len) != 0)
        return -1;

    /* 使用零字节产生调用方要求的完整 dummy 字节。 */
    for (uint8_t cycles = dummy_cycles; cycles != 0u; cycles -= 8u)
    {
        if (startysky_t1_pico_qspi_wait_tx_space() != 0)
            return -1;

        REG_QSPI_0_DR = 0u;
    }

    REG_QSPI_0_SER = startysky_t1_pico_qspi_chip_select(HAL_QSPI_CS_0);

    /* 逐字节等待并读取接收 FIFO。 */
    for (uint16_t index = 0u; index < rx_len; ++index)
    {
        if (startysky_t1_pico_qspi_wait_rx_data() != 0)
        {
            REG_QSPI_0_SER = 0u;
            return -1;
        }

        rx_buf[index] = (uint8_t)REG_QSPI_0_DR;
    }

    /* 等待事务完成并释放片选零。 */
    if (startysky_t1_pico_qspi_wait_idle() != 0)
    {
        REG_QSPI_0_SER = 0u;
        return -1;
    }

    REG_QSPI_0_SER = 0u;
    return 0;
}
