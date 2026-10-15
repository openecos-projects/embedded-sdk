#include "hal_qspi.h"
#include "qspi.h"

int hal_qspi_init(hal_qspi_port_t port, const hal_qspi_config_t *config) {
    if (port != HAL_QSPI_PORT_0 || config == NULL) {
        return -1;
    }
    qspi_config_t legacy_config = {.clkdiv = config->clkdiv};
    qspi_init(&legacy_config);
    return 0;
}

int hal_qspi_deinit(hal_qspi_port_t port) {
    return port == HAL_QSPI_PORT_0 ? 0 : -1;
}

int hal_qspi_write_8_cs(hal_qspi_port_t port, uint8_t data,
                        hal_qspi_cs_t cs) {
    (void)cs;
    if (port != HAL_QSPI_PORT_0) return -1;
    qspi_write_8(data);
    return 0;
}

int hal_qspi_write_16_cs(hal_qspi_port_t port, uint16_t data,
                         hal_qspi_cs_t cs) {
    (void)cs;
    if (port != HAL_QSPI_PORT_0) return -1;
    qspi_write_16(data);
    return 0;
}

int hal_qspi_write_32_cs(hal_qspi_port_t port, uint32_t data,
                         hal_qspi_cs_t cs) {
    (void)cs;
    if (port != HAL_QSPI_PORT_0) return -1;
    qspi_write_32(data);
    return 0;
}

int hal_qspi_write_32x2_cs(hal_qspi_port_t port, uint32_t data1,
                           uint32_t data2, hal_qspi_cs_t cs) {
    (void)cs;
    if (port != HAL_QSPI_PORT_0) return -1;
    qspi_write_32x2(data1, data2);
    return 0;
}

int hal_qspi_write_32x8_cs(hal_qspi_port_t port, uint32_t data1,
                           uint32_t data2, uint32_t data3, uint32_t data4,
                           uint32_t data5, uint32_t data6, uint32_t data7,
                           uint32_t data8, hal_qspi_cs_t cs) {
    (void)cs;
    if (port != HAL_QSPI_PORT_0) return -1;
    qspi_write_32x8(data1, data2, data3, data4, data5, data6, data7, data8);
    return 0;
}

int hal_qspi_write_32x16_cs(
    hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3,
    uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7,
    uint32_t data8, uint32_t data9, uint32_t data10, uint32_t data11,
    uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15,
    uint32_t data16, hal_qspi_cs_t cs) {
    (void)cs;
    if (port != HAL_QSPI_PORT_0) return -1;
    qspi_write_32x16(data1, data2, data3, data4, data5, data6, data7, data8,
                     data9, data10, data11, data12, data13, data14, data15,
                     data16);
    return 0;
}

int hal_qspi_write_32x32_cs(
    hal_qspi_port_t port, uint32_t data1, uint32_t data2, uint32_t data3,
    uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7,
    uint32_t data8, uint32_t data9, uint32_t data10, uint32_t data11,
    uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15,
    uint32_t data16, uint32_t data17, uint32_t data18, uint32_t data19,
    uint32_t data20, uint32_t data21, uint32_t data22, uint32_t data23,
    uint32_t data24, uint32_t data25, uint32_t data26, uint32_t data27,
    uint32_t data28, uint32_t data29, uint32_t data30, uint32_t data31,
    uint32_t data32, hal_qspi_cs_t cs) {
    (void)cs;
    if (port != HAL_QSPI_PORT_0) return -1;
    qspi_write_32x32(data1, data2, data3, data4, data5, data6, data7, data8,
                     data9, data10, data11, data12, data13, data14, data15,
                     data16, data17, data18, data19, data20, data21, data22,
                     data23, data24, data25, data26, data27, data28, data29,
                     data30, data31, data32);
    return 0;
}
