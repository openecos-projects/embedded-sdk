#include "hal_qspi.h"
#include "board.h"
#include <stdio.h>

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
    REG_QSPI_0_STATUS = cs | 2;
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
    REG_QSPI_0_STATUS = cs | 2;
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
    REG_QSPI_0_STATUS = cs | 2;
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
    REG_QSPI_0_STATUS = cs | 2;
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
    REG_QSPI_0_STATUS = cs | 2;
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
    REG_QSPI_0_STATUS = cs | 2;
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
    REG_QSPI_0_STATUS = cs | 2;
    while ((REG_QSPI_0_STATUS & 0xFFFFFFFF) != 1)
        ;
    return 0;
}
