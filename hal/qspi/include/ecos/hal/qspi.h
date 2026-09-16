#ifndef ECOS_HAL_QSPI_H
#define ECOS_HAL_QSPI_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_QSPI_PORT_0 = 0,
    HAL_QSPI_PORT_MAX
} hal_qspi_port_t;

typedef enum {
    HAL_QSPI_CS_0 = 1 << 16,
    HAL_QSPI_CS_1 = 2 << 16,
    HAL_QSPI_CS_2 = 3 << 16,
    HAL_QSPI_CS_3 = 4 << 16,
} hal_qspi_cs_t;

typedef struct {
    uint32_t clkdiv;
} hal_qspi_config_t;

int hal_qspi_init(hal_qspi_port_t port, const hal_qspi_config_t *config);
int hal_qspi_deinit(hal_qspi_port_t port);

int hal_qspi_write_8(hal_qspi_port_t port, uint8_t data);
int hal_qspi_write_16(hal_qspi_port_t port, uint16_t data);
int hal_qspi_write_32(hal_qspi_port_t port, uint32_t data);
int hal_qspi_write_32_repeat(hal_qspi_port_t port, uint32_t data,
                             uint32_t words);
int hal_qspi_write_32x2(hal_qspi_port_t port, uint32_t data1, uint32_t data2);
int hal_qspi_write_32x8(hal_qspi_port_t port, uint32_t data1, uint32_t data2,
                        uint32_t data3, uint32_t data4, uint32_t data5,
                        uint32_t data6, uint32_t data7, uint32_t data8);
int hal_qspi_write_32x16(hal_qspi_port_t port, uint32_t data1, uint32_t data2,
                         uint32_t data3, uint32_t data4, uint32_t data5,
                         uint32_t data6, uint32_t data7, uint32_t data8,
                         uint32_t data9, uint32_t data10, uint32_t data11,
                         uint32_t data12, uint32_t data13, uint32_t data14,
                         uint32_t data15, uint32_t data16);
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
                         uint32_t data30, uint32_t data31, uint32_t data32);

int hal_qspi_write_8_cs(hal_qspi_port_t port, uint8_t data, hal_qspi_cs_t cs);
int hal_qspi_write_16_cs(hal_qspi_port_t port, uint16_t data, hal_qspi_cs_t cs);
int hal_qspi_write_32_cs(hal_qspi_port_t port, uint32_t data, hal_qspi_cs_t cs);
int hal_qspi_write_32x2_cs(hal_qspi_port_t port, uint32_t data1,
                           uint32_t data2, hal_qspi_cs_t cs);
int hal_qspi_write_32x8_cs(hal_qspi_port_t port, uint32_t data1,
                           uint32_t data2, uint32_t data3, uint32_t data4,
                           uint32_t data5, uint32_t data6, uint32_t data7,
                           uint32_t data8, hal_qspi_cs_t cs);
int hal_qspi_write_32x16_cs(hal_qspi_port_t port, uint32_t data1,
                            uint32_t data2, uint32_t data3, uint32_t data4,
                            uint32_t data5, uint32_t data6, uint32_t data7,
                            uint32_t data8, uint32_t data9, uint32_t data10,
                            uint32_t data11, uint32_t data12, uint32_t data13,
                            uint32_t data14, uint32_t data15, uint32_t data16,
                            hal_qspi_cs_t cs);
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
                            uint32_t data32, hal_qspi_cs_t cs);

int hal_qspi_send_cmd(hal_qspi_port_t port,
                      uint8_t cmd, uint8_t cmd_len,
                      uint32_t addr, uint8_t addr_len);
int hal_qspi_write(hal_qspi_port_t port,
                   uint8_t cmd, uint8_t cmd_len,
                   uint32_t addr, uint8_t addr_len,
                   const uint8_t *tx_buf, uint16_t tx_len);
int hal_qspi_read(hal_qspi_port_t port,
                  uint8_t cmd, uint8_t cmd_len,
                  uint32_t addr, uint8_t addr_len,
                  uint8_t dummy_cycles,
                  uint8_t *rx_buf, uint16_t rx_len);

#ifdef __cplusplus
}
#endif

#endif
