#include "hal_qspi.h"
#include "board.h"
#include <stdio.h>

static inline uint32_t get_hw_cs(hal_qspi_cs_t cs) {
    if (cs == HAL_QSPI_CS_0) return 1 << 8;
    if (cs == HAL_QSPI_CS_1) return 2 << 8;
    if (cs == HAL_QSPI_CS_2) return 3 << 8;
    if (cs == HAL_QSPI_CS_3) return 4 << 8;
    return 1 << 8;
}

int hal_qspi_init(hal_qspi_port_t port, const hal_qspi_config_t *config){
    if (port != HAL_QSPI_PORT_0 || !config) return -1;
    REG_QSPI_0_STATUS = (uint32_t)0b10000;
    REG_QSPI_0_STATUS = (uint32_t)0b00000;
    REG_QSPI_0_INTCFG = (uint32_t)0b00000;
    REG_QSPI_0_DUM = (uint32_t)0;
    REG_QSPI_0_CLKDIV = config->clkdiv; // sck = apb_clk/2(div+1)
    return 0;
}

int hal_qspi_deinit(hal_qspi_port_t port) {
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_STATUS = (uint32_t)0b10000;
    return 0;
}

/* 旧API */

int hal_qspi_write_8(hal_qspi_port_t port, uint8_t data){
    if (port != HAL_QSPI_PORT_0) return -1;
    uint32_t wdat = ((uint32_t)data) << 24;
    REG_QSPI_0_LEN = 0x80000;
    REG_QSPI_0_TXFIFO = wdat;
    REG_QSPI_0_STATUS = 258;
    while ((REG_QSPI_0_STATUS & 0xFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_8_cs(hal_qspi_port_t port, uint8_t data, hal_qspi_cs_t cs){
    if (port != HAL_QSPI_PORT_0) return -1;
    uint32_t wdat = ((uint32_t)data) << 24;
    REG_QSPI_0_LEN = 0x80000;
    REG_QSPI_0_TXFIFO = wdat;
    REG_QSPI_0_STATUS = get_hw_cs(cs) | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_16(hal_qspi_port_t port, uint16_t data){
    if (port != HAL_QSPI_PORT_0) return -1;
    uint32_t wdat = ((uint32_t)data) << 16;
    REG_QSPI_0_LEN = 0x100000;
    REG_QSPI_0_TXFIFO = wdat;
    REG_QSPI_0_STATUS = 258;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_16_cs(hal_qspi_port_t port, uint16_t data, hal_qspi_cs_t cs){
    if (port != HAL_QSPI_PORT_0) return -1;
    uint32_t wdat = ((uint32_t)data) << 16;
    REG_QSPI_0_LEN = 0x100000;
    REG_QSPI_0_TXFIFO = wdat;
    REG_QSPI_0_STATUS = get_hw_cs(cs) | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32(hal_qspi_port_t port, uint32_t data){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x200000;
    REG_QSPI_0_TXFIFO = data;
    REG_QSPI_0_STATUS = 258;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32_cs(hal_qspi_port_t port, uint32_t data, hal_qspi_cs_t cs){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x200000;
    REG_QSPI_0_TXFIFO = data;
    REG_QSPI_0_STATUS = get_hw_cs(cs) | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x2(hal_qspi_port_t port, uint32_t data1, uint32_t data2){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x400000;
    REG_QSPI_0_TXFIFO = data1;
    REG_QSPI_0_TXFIFO = data2;
    REG_QSPI_0_STATUS = 258;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x2_cs(hal_qspi_port_t port, uint32_t data1, uint32_t data2, hal_qspi_cs_t cs){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x400000;
    REG_QSPI_0_TXFIFO = data1;
    REG_QSPI_0_TXFIFO = data2;
    REG_QSPI_0_STATUS = get_hw_cs(cs) | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x8(hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x1000000;
    REG_QSPI_0_TXFIFO = data1;
    REG_QSPI_0_TXFIFO = data2;
    REG_QSPI_0_TXFIFO = data3;
    REG_QSPI_0_TXFIFO = data4;
    REG_QSPI_0_TXFIFO = data5;
    REG_QSPI_0_TXFIFO = data6;
    REG_QSPI_0_TXFIFO = data7;
    REG_QSPI_0_TXFIFO = data8;
    REG_QSPI_0_STATUS = 258;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x8_cs(hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8, hal_qspi_cs_t cs){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x1000000;
    REG_QSPI_0_TXFIFO = data1;
    REG_QSPI_0_TXFIFO = data2;
    REG_QSPI_0_TXFIFO = data3;
    REG_QSPI_0_TXFIFO = data4;
    REG_QSPI_0_TXFIFO = data5;
    REG_QSPI_0_TXFIFO = data6;
    REG_QSPI_0_TXFIFO = data7;
    REG_QSPI_0_TXFIFO = data8;
    REG_QSPI_0_STATUS = get_hw_cs(cs) | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x16(hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x2000000;
    REG_QSPI_0_TXFIFO = data1;
    REG_QSPI_0_TXFIFO = data2;
    REG_QSPI_0_TXFIFO = data3;
    REG_QSPI_0_TXFIFO = data4;
    REG_QSPI_0_TXFIFO = data5;
    REG_QSPI_0_TXFIFO = data6;
    REG_QSPI_0_TXFIFO = data7;
    REG_QSPI_0_TXFIFO = data8;
    REG_QSPI_0_TXFIFO = data9;
    REG_QSPI_0_TXFIFO = data10;
    REG_QSPI_0_TXFIFO = data11;
    REG_QSPI_0_TXFIFO = data12;
    REG_QSPI_0_TXFIFO = data13;
    REG_QSPI_0_TXFIFO = data14;
    REG_QSPI_0_TXFIFO = data15;
    REG_QSPI_0_TXFIFO = data16;
    REG_QSPI_0_STATUS = 258;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x16_cs(hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16, hal_qspi_cs_t cs){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x2000000;
    REG_QSPI_0_TXFIFO = data1;
    REG_QSPI_0_TXFIFO = data2;
    REG_QSPI_0_TXFIFO = data3;
    REG_QSPI_0_TXFIFO = data4;
    REG_QSPI_0_TXFIFO = data5;
    REG_QSPI_0_TXFIFO = data6;
    REG_QSPI_0_TXFIFO = data7;
    REG_QSPI_0_TXFIFO = data8;
    REG_QSPI_0_TXFIFO = data9;
    REG_QSPI_0_TXFIFO = data10;
    REG_QSPI_0_TXFIFO = data11;
    REG_QSPI_0_TXFIFO = data12;
    REG_QSPI_0_TXFIFO = data13;
    REG_QSPI_0_TXFIFO = data14;
    REG_QSPI_0_TXFIFO = data15;
    REG_QSPI_0_TXFIFO = data16;
    REG_QSPI_0_STATUS = get_hw_cs(cs) | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x32(hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16,
     uint32_t data17, uint32_t data18, uint32_t data19, uint32_t data20, uint32_t data21, uint32_t data22, uint32_t data23, uint32_t data24,
     uint32_t data25, uint32_t data26, uint32_t data27, uint32_t data28, uint32_t data29, uint32_t data30, uint32_t data31, uint32_t data32){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x4000000;
    REG_QSPI_0_TXFIFO = data1; REG_QSPI_0_TXFIFO = data2; REG_QSPI_0_TXFIFO = data3; REG_QSPI_0_TXFIFO = data4;
    REG_QSPI_0_TXFIFO = data5; REG_QSPI_0_TXFIFO = data6; REG_QSPI_0_TXFIFO = data7; REG_QSPI_0_TXFIFO = data8;
    REG_QSPI_0_TXFIFO = data9; REG_QSPI_0_TXFIFO = data10; REG_QSPI_0_TXFIFO = data11; REG_QSPI_0_TXFIFO = data12;
    REG_QSPI_0_TXFIFO = data13; REG_QSPI_0_TXFIFO = data14; REG_QSPI_0_TXFIFO = data15; REG_QSPI_0_TXFIFO = data16;
    REG_QSPI_0_TXFIFO = data17; REG_QSPI_0_TXFIFO = data18; REG_QSPI_0_TXFIFO = data19; REG_QSPI_0_TXFIFO = data20;
    REG_QSPI_0_TXFIFO = data21; REG_QSPI_0_TXFIFO = data22; REG_QSPI_0_TXFIFO = data23; REG_QSPI_0_TXFIFO = data24;
    REG_QSPI_0_TXFIFO = data25; REG_QSPI_0_TXFIFO = data26; REG_QSPI_0_TXFIFO = data27; REG_QSPI_0_TXFIFO = data28;
    REG_QSPI_0_TXFIFO = data29; REG_QSPI_0_TXFIFO = data30; REG_QSPI_0_TXFIFO = data31; REG_QSPI_0_TXFIFO = data32;
    REG_QSPI_0_STATUS = 258;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}

int hal_qspi_write_32x32_cs(hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16,
     uint32_t data17, uint32_t data18, uint32_t data19, uint32_t data20, uint32_t data21, uint32_t data22, uint32_t data23, uint32_t data24,
     uint32_t data25, uint32_t data26, uint32_t data27, uint32_t data28, uint32_t data29, uint32_t data30, uint32_t data31, uint32_t data32, hal_qspi_cs_t cs){
    if (port != HAL_QSPI_PORT_0) return -1;
    REG_QSPI_0_LEN = 0x4000000;
    REG_QSPI_0_TXFIFO = data1; REG_QSPI_0_TXFIFO = data2; REG_QSPI_0_TXFIFO = data3; REG_QSPI_0_TXFIFO = data4;
    REG_QSPI_0_TXFIFO = data5; REG_QSPI_0_TXFIFO = data6; REG_QSPI_0_TXFIFO = data7; REG_QSPI_0_TXFIFO = data8;
    REG_QSPI_0_TXFIFO = data9; REG_QSPI_0_TXFIFO = data10; REG_QSPI_0_TXFIFO = data11; REG_QSPI_0_TXFIFO = data12;
    REG_QSPI_0_TXFIFO = data13; REG_QSPI_0_TXFIFO = data14; REG_QSPI_0_TXFIFO = data15; REG_QSPI_0_TXFIFO = data16;
    REG_QSPI_0_TXFIFO = data17; REG_QSPI_0_TXFIFO = data18; REG_QSPI_0_TXFIFO = data19; REG_QSPI_0_TXFIFO = data20;
    REG_QSPI_0_TXFIFO = data21; REG_QSPI_0_TXFIFO = data22; REG_QSPI_0_TXFIFO = data23; REG_QSPI_0_TXFIFO = data24;
    REG_QSPI_0_TXFIFO = data25; REG_QSPI_0_TXFIFO = data26; REG_QSPI_0_TXFIFO = data27; REG_QSPI_0_TXFIFO = data28;
    REG_QSPI_0_TXFIFO = data29; REG_QSPI_0_TXFIFO = data30; REG_QSPI_0_TXFIFO = data31; REG_QSPI_0_TXFIFO = data32;
    REG_QSPI_0_STATUS = get_hw_cs(cs) | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}


/* 新API */
/* ========== FSM 状态位 (读 REG_STATUS 返回) ========== */
#define FSM_IDLE    0x01  // bit[0]
#define FSM_CMD     0x02  // bit[1]
#define FSM_ADDR    0x04  // bit[2]
#define FSM_DUMMY   0x10  // bit[4]
#define FSM_DATA_TX 0x20  // bit[5]
#define FSM_DATA_RX 0x40  // bit[6]

/* STATUS 写控制位 */
#define STATUS_SPI_RD  (1 << 0)
#define STATUS_SPI_WR  (1 << 1)
#define STATUS_SPI_QWR (1 << 3)
#define STATUS_CS0     (1 << 8)   // 固定 CS0

/* 硬件 FIFO 深度 (BUFFER_DEPTH=10) */
#define FIFO_DEPTH     10
/* 最大单次预填字节数 = FIFO_DEPTH * 4 */
#define MAX_PREFILL    (FIFO_DEPTH * 4)

/* ---------- 内部辅助函数 ---------- */

/**
 * @brief 忙等传输完成
 */
// #define QSPI_BUSY_BIT    (1 << 0)
static inline void qspi_wait_idle(void) {
    while ((REG_QSPI_0_STATUS & 0x7F) != FSM_IDLE); // TODO: 加超时保护
}

/**
 * @brief 将 4 字节打包为 32-bit 大端字
 *        硬件 MSB-first 移位：bit[31] 先出，bit[0] 最后出
 */
static inline uint32_t pack_word(const uint8_t *buf) {
    return ((uint32_t)buf[0] << 24)
         | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] << 8)
         |  (uint32_t)buf[3];
}

/**
 * @brief 将 32-bit 字解包为 4 字节（MSB first — 硬件左移，首 bit 到高位）
 *        data_int <<= 1; data_int[0] = sdi1
 *        → 24-bit 后 data[23:16]=首字节, data[7:0]=末字节
 *        buf[0] = word[31:24] (最先收到的字节, 4-byte 读)
 *        buf[3] = word[7:0]   (最后收到的字节)
 */
static inline void unpack_word(uint32_t word, uint8_t *buf) {
    buf[0] = (word >> 24) & 0xFF;
    buf[1] = (word >> 16) & 0xFF;
    buf[2] = (word >> 8) & 0xFF;
    buf[3] = word & 0xFF;
}

/**
 * @brief 从 32-bit 字中提取 N 字节（N < 4，尾字场景）
 *        N 字节右对齐在 word[ (N*8-1) : 0 ]，首字节在高位
 */
static inline void unpack_tail(uint32_t word, uint8_t *buf, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        buf[i] = (word >> ((n - 1 - i) * 8)) & 0xFF;
    }
}

/* ================================================================
 *  hal_qspi_send_cmd — 命令 + 地址阶段（无数据阶段）
 *
 *  流程: SPICMD → SPIADR → SPILEN → STATUS(触发) → 轮询 IDLE
 *  cmd_len/addr_len 为 0 时自动跳过对应阶段
 * ================================================================ */
int hal_qspi_send_cmd(hal_qspi_port_t port,
                       uint8_t cmd, uint8_t cmd_len,
                       uint32_t addr, uint8_t addr_len)
{
    if (port != HAL_QSPI_PORT_0) return -1;

    /* 命令字左移对齐到 MSB（硬件从 bit[31] 开始移位） */
    if (cmd_len > 0) {
        REG_QSPI_0_CMD = (uint32_t)cmd << (32 - cmd_len);
    }
    /* 地址左移对齐 */
    if (addr_len > 0) {
        REG_QSPI_0_ADR = addr << (32 - addr_len);
    }
    /* 配置各阶段长度 — 数据长度 = 0 */
    REG_QSPI_0_LEN = ((uint32_t)addr_len << 8)
                   | ((uint32_t)cmd_len);

    /* 触发传输: spi_rd 模式 (只发不收), CS0 */
    REG_QSPI_0_STATUS = STATUS_SPI_RD | STATUS_CS0;

    /* 等待传输完成 */
    qspi_wait_idle();
    return 0;
}

/* ================================================================
 *  hal_qspi_write — 命令 + 地址 + 写数据
 *
 *  流程: 预填 TXFIFO → SPICMD/SPIADR/SPILEN → STATUS(触发) →
 *        边传边填 FIFO → 轮询 IDLE
 * ================================================================ */
int hal_qspi_write(hal_qspi_port_t port,
                    uint8_t cmd, uint8_t cmd_len,
                    uint32_t addr, uint8_t addr_len,
                    const uint8_t *tx_buf, uint16_t tx_len)
{
    if (port != HAL_QSPI_PORT_0) return -1;
    if (tx_len == 0) {
        /* 无数据 → 退化为 send_cmd */
        return hal_qspi_send_cmd(port, cmd, cmd_len, addr, addr_len);
    }

    uint16_t bytes_remaining = tx_len;
    const uint8_t *p = tx_buf;

    /* --- 1. 预填 TXFIFO（最多 40 字节 / 10 字，仅填完整字）--- */
    uint8_t prefill_bytes = (tx_len < MAX_PREFILL) ? (uint8_t)(tx_len & ~3u)
                                                   : MAX_PREFILL;
    for (uint8_t i = 0; i < prefill_bytes; i += 4) {
        REG_QSPI_0_TXFIFO = pack_word(p);
        p += 4;
        bytes_remaining -= 4;
    }

    /* --- 2. 配置传输参数 --- */
    if (cmd_len > 0) {
        REG_QSPI_0_CMD = (uint32_t)cmd << (32 - cmd_len);
    }
    if (addr_len > 0) {
        REG_QSPI_0_ADR = addr << (32 - addr_len);
    }
    /* data_len 以 bit 为单位 */
    REG_QSPI_0_LEN = ((uint32_t)(tx_len * 8) << 16)
                   | ((uint32_t)addr_len << 8)
                   | ((uint32_t)cmd_len);

    /* --- 3. 触发传输 --- */
    REG_QSPI_0_STATUS = STATUS_SPI_WR | STATUS_CS0;

    /* --- 4. 边传边填剩余数据 --- */
    while (bytes_remaining > 0) {
        /* 等 FSM 进入 DATA_TX 状态且 FIFO 有空位 */
        if ((REG_QSPI_0_STATUS & 0x7F) == FSM_DATA_TX) {
            if (bytes_remaining >= 4) {
                REG_QSPI_0_TXFIFO = pack_word(p);
                p += 4;
                bytes_remaining -= 4;
            } else {
                /* 尾字不足 4 字节，零填充打包，避免越界读 */
                uint8_t tail[4] = {0};
                for (uint8_t j = 0; j < bytes_remaining; j++) {
                    tail[j] = p[j];
                }
                REG_QSPI_0_TXFIFO = pack_word(tail);
                bytes_remaining = 0;
            }
        }
    }

    /* --- 5. 等待传输完成 --- */
    qspi_wait_idle();
    return 0;
}

/* ================================================================
 *  hal_qspi_read — 命令 + 地址 + dummy + 读数据
 *
 *  流程: SPICMD/SPIADR/SPILEN/SPIDUM → STATUS(触发) →
 *        读 STATUS 的 elements_rx[20:16] 门控读 RXFIFO → IDLE
 *
 *  用 STATUS 寄存器的高位 elements_rx 计数替代 INTSTA，
 *  直接反映 RXFIFO 当前元素数，无阈值配置依赖。
 *  硬件在 RXFIFO 满时会自动停 SPI 时钟 (WAIT_FIFO)，不丢数据。
 * ================================================================ */
int hal_qspi_read(hal_qspi_port_t port,
                   uint8_t cmd, uint8_t cmd_len,
                   uint32_t addr, uint8_t addr_len,
                   uint8_t dummy_cycles,
                   uint8_t *rx_buf, uint16_t rx_len)
{
    if (port != HAL_QSPI_PORT_0) return -1;
    if (rx_len == 0) return 0;

    uint8_t *p = rx_buf;

    /* 硬件 FIFO 深 10 字(40B)；满时 WAIT_FIFO 有粘滞 bug（bit21 不释放），
     * 故每笔传输 ≤32B，永远不触发 FIFO-full → 先等 IDLE 再排水是安全的。 */
#define SAFE_CHUNK 32

    while (rx_len > 0) {
        uint16_t chunk = (rx_len > SAFE_CHUNK) ? SAFE_CHUNK : rx_len;
        uint16_t chunk_remaining = chunk;

        /* --- 1. 配置传输参数 --- */
        if (cmd_len > 0) {
            REG_QSPI_0_CMD = (uint32_t)cmd << (32 - cmd_len);
        }
        if (addr_len > 0) {
            REG_QSPI_0_ADR = addr << (32 - addr_len);
        }
        REG_QSPI_0_LEN = ((uint32_t)(chunk * 8) << 16)
                       | ((uint32_t)addr_len << 8)
                       | ((uint32_t)cmd_len);
        REG_QSPI_0_DUM = (uint32_t)dummy_cycles;

        /* --- 2. 触发传输 --- */
        REG_QSPI_0_STATUS = STATUS_SPI_RD | STATUS_CS0;

        /* --- 3. 等 IDLE 后排水（chunk ≤ FIFO 深度，安全）--- */
        qspi_wait_idle();

        while (chunk_remaining > 0) {
            if (((REG_QSPI_0_STATUS >> 16) & 0x1F) == 0) break;
            uint32_t word = REG_QSPI_0_RXFIFO;
            if (chunk_remaining >= 4) {
                uint8_t wb[4];
                unpack_word(word, wb);
                for (uint8_t i = 0; i < 4; i++) *p++ = wb[i];
                chunk_remaining -= 4;
            } else {
                uint8_t wb[4];
                unpack_tail(word, wb, (uint8_t)chunk_remaining);
                for (uint8_t i = 0; i < chunk_remaining; i++) *p++ = wb[i];
                chunk_remaining = 0;
            }
        }

        rx_len -= chunk;
        if (addr_len > 0) {
            addr += chunk;
        }
    }

    return 0;
}