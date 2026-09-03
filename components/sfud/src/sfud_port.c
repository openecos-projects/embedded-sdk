/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface — hardware QSPI backend.
 */

#include <sfud.h>
#include "hal_qspi.h"
#include "hal_timer.h"

/* ================================================================
 *  spi_write_read — SFUD 核心 SPI 传输函数
 *
 *  write_buf 格式: [cmd, addr(opt), tx_data(opt)]
 *  根据 write_size / read_size 自动路由到 hal_qspi_* 函数。
 * ================================================================ */
static sfud_err spi_write_read(const sfud_spi *spi,
                               const uint8_t *write_buf, size_t write_size,
                               uint8_t *read_buf, size_t read_size)
{
    (void)spi;

    uint8_t  cmd       = write_buf[0];
    uint8_t  cmd_len   = 8;
    uint8_t  addr_len  = 0;
    uint32_t addr      = 0;

    /* --- 解析地址 ---
     * SFUD 对带地址的命令，write_buf[1..] 是 3 或 4 字节地址。
     * write_size == 1 表示纯命令（如 0x06 WREN, 0x9F JDEC ID, 0x05 RDSR）。
     * write_size >= 4 表示命令 + 3 字节地址（或 >=5 表示 4 字节地址）。
     * 简单判定: write_size > 1 且 write_size 足够容纳地址时，提取地址。
     */
    if (write_size > 1) {
        /* 默认 3 字节地址 (24-bit); 4 字节模式可后续通过 flash->addr_in_4_byte 扩展 */
        addr_len = 24;
        uint8_t addr_bytes = addr_len / 8;

        if (write_size < (size_t)(1 + addr_bytes)) {
            /* write_size 不足一个完整地址 → 可能是不带地址的短命令 */
            addr_len = 0;
        } else {
            for (uint8_t i = 0; i < addr_bytes; i++) {
                addr = (addr << 8) | write_buf[1 + i];
            }
        }
    }

    /* --- 解析写数据（地址之后的字节）--- */
    uint8_t addr_bytes = addr_len / 8;
    size_t  tx_offset  = 1 + addr_bytes;
    const uint8_t *tx_data = (write_size > tx_offset) ? &write_buf[tx_offset] : NULL;
    uint16_t tx_len = (write_size > tx_offset) ? (uint16_t)(write_size - tx_offset) : 0;

    /* --- 路由到硬件 QSPI 函数 --- */
    if (read_size > 0) {
        /* 读操作: 命令 + 地址 + (dummy) + 读数据
         * SFUD 把 dummy 字节放在 write_buf 尾部，需转换为 dummy_cycles */
        uint8_t dummy_cycles = 0;
        if (tx_len > 0) {
            dummy_cycles = (uint8_t)(tx_len * 8);
        }
        return (sfud_err)hal_qspi_read(HAL_QSPI_PORT_0,
                                        cmd, cmd_len,
                                        addr, addr_len,
                                        dummy_cycles,
                                        read_buf, (uint16_t)read_size);
    } else if (tx_len > 0) {
        /* 写操作: 命令 + 地址 + 写数据 */
        return (sfud_err)hal_qspi_write(HAL_QSPI_PORT_0,
                                         cmd, cmd_len,
                                         addr, addr_len,
                                         tx_data, tx_len);
    } else {
        /* 纯命令: 如 Write Enable (0x06) */
        return (sfud_err)hal_qspi_send_cmd(HAL_QSPI_PORT_0,
                                            cmd, cmd_len,
                                            addr, addr_len);
    }
}

#ifdef SFUD_USING_QSPI
/**
 * QSPI fast read (stub — SFUD_USING_QSPI not yet enabled)
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr,
                          sfud_qspi_read_cmd_format *qspi_read_cmd_format,
                          uint8_t *read_buf, size_t read_size)
{
    /* TODO: 使用 hal_qspi_read 的 quad 模式 */
    (void)spi;
    (void)addr;
    (void)qspi_read_cmd_format;
    (void)read_buf;
    (void)read_size;
    return SFUD_ERR_READ;
}
#endif /* SFUD_USING_QSPI */

/* ================================================================
 *  sfud_spi_port_init — 初始化 SPI 端口
 * ================================================================ */
sfud_err sfud_spi_port_init(sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;

    /* --- 初始化硬件 QSPI --- */
    hal_qspi_config_t qspi_cfg = { .clkdiv = 4 };
    if (hal_qspi_init(HAL_QSPI_PORT_0, &qspi_cfg) != 0) {
        return SFUD_ERR_WRITE;
    }

    /* --- 注册回调 --- */
    flash->spi.wr   = spi_write_read;
    flash->spi.lock = NULL;
    flash->spi.unlock = NULL;
    flash->spi.user_data = NULL;

#ifdef SFUD_USING_QSPI
    flash->spi.qspi_read = qspi_read;
#endif

    /* --- 重试配置 --- */
    flash->retry.delay = NULL;
    flash->retry.times = 10000;

    return result;
}
