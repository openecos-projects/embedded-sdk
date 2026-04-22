#include "hal_qspi.h"
#include "board.h"
#include "generated/autoconf.h"


#define QSPI_BUSY_BIT    (1 << 0)


static void qspi_wait_busy(void){
    while (REG_QSPI_0_STATUS & QSPI_BUSY_BIT);
}

int hal_qspi_init(hal_qspi_port_t port, const hal_qspi_config_t *config){
    if (port >= HAL_QSPI_PORT_MAX || config == NULL)
        return -1;

    REG_QSPI_0_CLKDIV = config->clkdiv;

    REG_QSPI_0_CMD = 0;
    REG_QSPI_0_ADR = 0;
    REG_QSPI_0_LEN = 0;

    return 0;
}


int hal_qspi_deinit(hal_qspi_port_t port){
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    REG_QSPI_0_CLKDIV = 0;
    REG_QSPI_0_CMD = 0;
    REG_QSPI_0_ADR = 0;
    REG_QSPI_0_LEN = 0;

    return 0;
}

int hal_qspi_write_8(hal_qspi_port_t port, uint8_t data){
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_TXFIFO = data;
    return 0;
}

int hal_qspi_write_16(hal_qspi_port_t port, uint16_t data){
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_TXFIFO = data;
    return 0;
}

int hal_qspi_write_32(hal_qspi_port_t port, uint32_t data){
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_TXFIFO = data;
    return 0;
}

int hal_qspi_write_32x2(hal_qspi_port_t port, uint32_t data1, uint32_t data2){
    hal_qspi_write_32(port, data1);
    hal_qspi_write_32(port, data2);
    return 0;
}

int hal_qspi_write_32x8(hal_qspi_port_t port,
    uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4,
    uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8)
{
    hal_qspi_write_32(port, data1);
    hal_qspi_write_32(port, data2);
    hal_qspi_write_32(port, data3);
    hal_qspi_write_32(port, data4);
    hal_qspi_write_32(port, data5);
    hal_qspi_write_32(port, data6);
    hal_qspi_write_32(port, data7);
    hal_qspi_write_32(port, data8);
    return 0;
}

int hal_qspi_write_32x16(hal_qspi_port_t port,
    uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4,
    uint32_t d5, uint32_t d6, uint32_t d7, uint32_t d8,
    uint32_t d9, uint32_t d10, uint32_t d11, uint32_t d12,
    uint32_t d13, uint32_t d14, uint32_t d15, uint32_t d16)
{
    hal_qspi_write_32x8(port, d1, d2, d3, d4, d5, d6, d7, d8);
    hal_qspi_write_32x8(port, d9, d10, d11, d12, d13, d14, d15, d16);
    return 0;
}

int hal_qspi_write_32x32(hal_qspi_port_t port,
    uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4,
    uint32_t d5, uint32_t d6, uint32_t d7, uint32_t d8,
    uint32_t d9, uint32_t d10, uint32_t d11, uint32_t d12,
    uint32_t d13, uint32_t d14, uint32_t d15, uint32_t d16,
    uint32_t d17, uint32_t d18, uint32_t d19, uint32_t d20,
    uint32_t d21, uint32_t d22, uint32_t d23, uint32_t d24,
    uint32_t d25, uint32_t d26, uint32_t d27, uint32_t d28,
    uint32_t d29, uint32_t d30, uint32_t d31, uint32_t d32)
{
    hal_qspi_write_32x16(port, d1, d2, d3, d4, d5, d6, d7, d8,
        d9, d10, d11, d12, d13, d14, d15, d16);
    hal_qspi_write_32x16(port, d17, d18, d19, d20, d21, d22, d23, d24,
        d25, d26, d27, d28, d29, d30, d31, d32);
    return 0;
}


int hal_qspi_write_8_cs(hal_qspi_port_t port, uint8_t data, hal_qspi_cs_t cs){
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_CMD = cs;
    REG_QSPI_0_TXFIFO = data;
    return 0;
}

int hal_qspi_write_16_cs(hal_qspi_port_t port, uint16_t data, hal_qspi_cs_t cs){
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_CMD = cs;
    REG_QSPI_0_TXFIFO = data;
    return 0;
}

int hal_qspi_write_32_cs(hal_qspi_port_t port, uint32_t data, hal_qspi_cs_t cs){
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_CMD = cs;
    REG_QSPI_0_TXFIFO = data;
    return 0;
}

int hal_qspi_write_32x2_cs(hal_qspi_port_t port,
    uint32_t data1, uint32_t data2, hal_qspi_cs_t cs)
{
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_CMD = cs;
    hal_qspi_write_32(port, data1);
    hal_qspi_write_32(port, data2);
    return 0;
}

int hal_qspi_write_32x8_cs(hal_qspi_port_t port,
    uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4,
    uint32_t d5, uint32_t d6, uint32_t d7, uint32_t d8, hal_qspi_cs_t cs)
{
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_CMD = cs;
    hal_qspi_write_32x8(port, d1, d2, d3, d4, d5, d6, d7, d8);
    return 0;
}

int hal_qspi_write_32x16_cs(hal_qspi_port_t port,
    uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4,
    uint32_t d5, uint32_t d6, uint32_t d7, uint32_t d8,
    uint32_t d9, uint32_t d10, uint32_t d11, uint32_t d12,
    uint32_t d13, uint32_t d14, uint32_t d15, uint32_t d16, hal_qspi_cs_t cs)
{
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_CMD = cs;
    hal_qspi_write_32x16(port,
        d1, d2, d3, d4, d5, d6, d7, d8,
        d9, d10, d11, d12, d13, d14, d15, d16);
    return 0;
}

int hal_qspi_write_32x32_cs(hal_qspi_port_t port,
    uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4,
    uint32_t d5, uint32_t d6, uint32_t d7, uint32_t d8,
    uint32_t d9, uint32_t d10, uint32_t d11, uint32_t d12,
    uint32_t d13, uint32_t d14, uint32_t d15, uint32_t d16,
    uint32_t d17, uint32_t d18, uint32_t d19, uint32_t d20,
    uint32_t d21, uint32_t d22, uint32_t d23, uint32_t d24,
    uint32_t d25, uint32_t d26, uint32_t d27, uint32_t d28,
    uint32_t d29, uint32_t d30, uint32_t d31, uint32_t d32, hal_qspi_cs_t cs)
{
    if (port >= HAL_QSPI_PORT_MAX)
        return -1;

    qspi_wait_busy();
    REG_QSPI_0_CMD = cs;
    hal_qspi_write_32x32(port,
        d1, d2, d3, d4, d5, d6, d7, d8,
        d9, d10, d11, d12, d13, d14, d15, d16,
        d17, d18, d19, d20, d21, d22, d23, d24,
        d25, d26, d27, d28, d29, d30, d31, d32);
    return 0;
}