#ifndef ECOS_DRIVER_QSPI_H
#define ECOS_DRIVER_QSPI_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_qspi_id_t;

#define ECOS_QSPI_DEFAULT ((ecos_qspi_id_t)0u)

typedef enum {
    ECOS_QSPI_CS_0 = 0,
    ECOS_QSPI_CS_1,
    ECOS_QSPI_CS_2,
    ECOS_QSPI_CS_3,
    ECOS_QSPI_CS_COUNT
} ecos_qspi_cs_t;

typedef struct {
    uint32_t clock_divider;
} ecos_qspi_config_t;

#define ECOS_QSPI_CONFIG_DEFAULT \
    { 0u }

/* Returns the number of QSPI controller instances provided by the Target. */
int ecos_qspi_get_instance_count(void);

ecos_err_t ecos_qspi_init(ecos_qspi_id_t qspi,
                          const ecos_qspi_config_t *config);
ecos_err_t ecos_qspi_deinit(ecos_qspi_id_t qspi);

/* Frame-oriented writes. Each call keeps one chip-select assertion. */
ecos_err_t ecos_qspi_write_8(ecos_qspi_id_t qspi, uint8_t data);
ecos_err_t ecos_qspi_write_16(ecos_qspi_id_t qspi, uint16_t data);
ecos_err_t ecos_qspi_write_32(ecos_qspi_id_t qspi, uint32_t data);
ecos_err_t ecos_qspi_write_32_repeat(ecos_qspi_id_t qspi, uint32_t data,
                                     uint32_t words);
ecos_err_t ecos_qspi_write_32x2(ecos_qspi_id_t qspi,
                                uint32_t data1, uint32_t data2);
ecos_err_t ecos_qspi_write_32x8(ecos_qspi_id_t qspi,
                                uint32_t data1, uint32_t data2,
                                uint32_t data3, uint32_t data4,
                                uint32_t data5, uint32_t data6,
                                uint32_t data7, uint32_t data8);
ecos_err_t ecos_qspi_write_32x16(ecos_qspi_id_t qspi,
                                 uint32_t data1, uint32_t data2,
                                 uint32_t data3, uint32_t data4,
                                 uint32_t data5, uint32_t data6,
                                 uint32_t data7, uint32_t data8,
                                 uint32_t data9, uint32_t data10,
                                 uint32_t data11, uint32_t data12,
                                 uint32_t data13, uint32_t data14,
                                 uint32_t data15, uint32_t data16);
ecos_err_t ecos_qspi_write_32x32(ecos_qspi_id_t qspi,
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
                                 uint32_t data31, uint32_t data32);

ecos_err_t ecos_qspi_write_8_cs(ecos_qspi_id_t qspi, uint8_t data,
                                ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_16_cs(ecos_qspi_id_t qspi, uint16_t data,
                                 ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_32_cs(ecos_qspi_id_t qspi, uint32_t data,
                                 ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_32x2_cs(ecos_qspi_id_t qspi,
                                   uint32_t data1, uint32_t data2,
                                   ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_32x8_cs(ecos_qspi_id_t qspi,
                                   uint32_t data1, uint32_t data2,
                                   uint32_t data3, uint32_t data4,
                                   uint32_t data5, uint32_t data6,
                                   uint32_t data7, uint32_t data8,
                                   ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_32x16_cs(ecos_qspi_id_t qspi,
                                    uint32_t data1, uint32_t data2,
                                    uint32_t data3, uint32_t data4,
                                    uint32_t data5, uint32_t data6,
                                    uint32_t data7, uint32_t data8,
                                    uint32_t data9, uint32_t data10,
                                    uint32_t data11, uint32_t data12,
                                    uint32_t data13, uint32_t data14,
                                    uint32_t data15, uint32_t data16,
                                    ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_32x32_cs(ecos_qspi_id_t qspi,
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
                                    ecos_qspi_cs_t cs);

/* Command/address transactions use the target's default CS0 line. */
ecos_err_t ecos_qspi_send_cmd(ecos_qspi_id_t qspi,
                              uint8_t command, uint8_t command_bits,
                              uint32_t address, uint8_t address_bits);
ecos_err_t ecos_qspi_write(ecos_qspi_id_t qspi,
                           uint8_t command, uint8_t command_bits,
                           uint32_t address, uint8_t address_bits,
                           const void *data, size_t size);
ecos_err_t ecos_qspi_read(ecos_qspi_id_t qspi,
                          uint8_t command, uint8_t command_bits,
                          uint32_t address, uint8_t address_bits,
                          uint8_t dummy_cycles, void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
