#include "ecos/driver/qspi.h"

#include "ecos/hal/qspi.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static int qspi_id_is_valid(ecos_qspi_id_t qspi)
{
    return qspi < HAL_QSPI_PORT_MAX;
}

static int qspi_cs_is_valid(ecos_qspi_cs_t cs)
{
    return cs >= ECOS_QSPI_CS_0 && cs < ECOS_QSPI_CS_COUNT;
}

static hal_qspi_cs_t qspi_hal_cs(ecos_qspi_cs_t cs)
{
    switch (cs) {
    case ECOS_QSPI_CS_0:
        return HAL_QSPI_CS_0;
    case ECOS_QSPI_CS_1:
        return HAL_QSPI_CS_1;
    case ECOS_QSPI_CS_2:
        return HAL_QSPI_CS_2;
    case ECOS_QSPI_CS_3:
        return HAL_QSPI_CS_3;
    default:
        return HAL_QSPI_CS_0;
    }
}

static ecos_err_t qspi_map_hal_result(int result)
{
    if (result == -1)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (result == -2)
        return ECOS_ERR_TIMEOUT;
    if (result >= 0)
        return (ecos_err_t)result;
    return ecos_err_is_known(result) ? (ecos_err_t)result : ECOS_ERR_IO;
}

static int qspi_validate_phase(uint8_t command_bits, uint8_t address_bits)
{
    return command_bits <= 8u && address_bits <= 32u;
}

static int qspi_validate_data(const void *data, size_t size)
{
    return (data != NULL || size == 0u) && size <= UINT16_MAX;
}

int ecos_qspi_get_instance_count(void)
{
    return (int)HAL_QSPI_PORT_MAX;
}

ecos_err_t ecos_qspi_init(ecos_qspi_id_t qspi,
                          const ecos_qspi_config_t *config)
{
    hal_qspi_config_t hal_config;

    if (!qspi_id_is_valid(qspi) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    hal_config.clkdiv = config->clock_divider;
    return qspi_map_hal_result(
        hal_qspi_init((hal_qspi_port_t)qspi, &hal_config)
    );
}

ecos_err_t ecos_qspi_deinit(ecos_qspi_id_t qspi)
{
    if (!qspi_id_is_valid(qspi))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_deinit((hal_qspi_port_t)qspi));
}

ecos_err_t ecos_qspi_write_8(ecos_qspi_id_t qspi, uint8_t data)
{
    return ecos_qspi_write_8_cs(qspi, data, ECOS_QSPI_CS_0);
}

ecos_err_t ecos_qspi_write_16(ecos_qspi_id_t qspi, uint16_t data)
{
    return ecos_qspi_write_16_cs(qspi, data, ECOS_QSPI_CS_0);
}

ecos_err_t ecos_qspi_write_32(ecos_qspi_id_t qspi, uint32_t data)
{
    return ecos_qspi_write_32_cs(qspi, data, ECOS_QSPI_CS_0);
}

ecos_err_t ecos_qspi_write_32_repeat(ecos_qspi_id_t qspi, uint32_t data,
                                     uint32_t words)
{
    if (!qspi_id_is_valid(qspi) || words == 0u || words > 32u)
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(
        hal_qspi_write_32_repeat((hal_qspi_port_t)qspi, data, words)
    );
}

ecos_err_t ecos_qspi_write_32x2(ecos_qspi_id_t qspi,
                                uint32_t data1, uint32_t data2)
{
    return ecos_qspi_write_32x2_cs(qspi, data1, data2, ECOS_QSPI_CS_0);
}

ecos_err_t ecos_qspi_write_32x8(ecos_qspi_id_t qspi,
                                uint32_t data1, uint32_t data2,
                                uint32_t data3, uint32_t data4,
                                uint32_t data5, uint32_t data6,
                                uint32_t data7, uint32_t data8)
{
    return ecos_qspi_write_32x8_cs(
        qspi, data1, data2, data3, data4, data5, data6, data7, data8,
        ECOS_QSPI_CS_0
    );
}

ecos_err_t ecos_qspi_write_32x16(ecos_qspi_id_t qspi,
                                 uint32_t data1, uint32_t data2,
                                 uint32_t data3, uint32_t data4,
                                 uint32_t data5, uint32_t data6,
                                 uint32_t data7, uint32_t data8,
                                 uint32_t data9, uint32_t data10,
                                 uint32_t data11, uint32_t data12,
                                 uint32_t data13, uint32_t data14,
                                 uint32_t data15, uint32_t data16)
{
    return ecos_qspi_write_32x16_cs(
        qspi, data1, data2, data3, data4, data5, data6, data7, data8,
        data9, data10, data11, data12, data13, data14, data15, data16,
        ECOS_QSPI_CS_0
    );
}

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
                                 uint32_t data31, uint32_t data32)
{
    return ecos_qspi_write_32x32_cs(
        qspi, data1, data2, data3, data4, data5, data6, data7, data8,
        data9, data10, data11, data12, data13, data14, data15, data16,
        data17, data18, data19, data20, data21, data22, data23, data24,
        data25, data26, data27, data28, data29, data30, data31, data32,
        ECOS_QSPI_CS_0
    );
}

ecos_err_t ecos_qspi_write_8_cs(ecos_qspi_id_t qspi, uint8_t data,
                                ecos_qspi_cs_t cs)
{
    if (!qspi_id_is_valid(qspi) || !qspi_cs_is_valid(cs))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(
        hal_qspi_write_8_cs((hal_qspi_port_t)qspi, data, qspi_hal_cs(cs))
    );
}

ecos_err_t ecos_qspi_write_16_cs(ecos_qspi_id_t qspi, uint16_t data,
                                 ecos_qspi_cs_t cs)
{
    if (!qspi_id_is_valid(qspi) || !qspi_cs_is_valid(cs))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(
        hal_qspi_write_16_cs((hal_qspi_port_t)qspi, data, qspi_hal_cs(cs))
    );
}

ecos_err_t ecos_qspi_write_32_cs(ecos_qspi_id_t qspi, uint32_t data,
                                 ecos_qspi_cs_t cs)
{
    if (!qspi_id_is_valid(qspi) || !qspi_cs_is_valid(cs))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(
        hal_qspi_write_32_cs((hal_qspi_port_t)qspi, data, qspi_hal_cs(cs))
    );
}

ecos_err_t ecos_qspi_write_32x2_cs(ecos_qspi_id_t qspi,
                                   uint32_t data1, uint32_t data2,
                                   ecos_qspi_cs_t cs)
{
    if (!qspi_id_is_valid(qspi) || !qspi_cs_is_valid(cs))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_write_32x2_cs(
        (hal_qspi_port_t)qspi, data1, data2, qspi_hal_cs(cs)
    ));
}

ecos_err_t ecos_qspi_write_32x8_cs(ecos_qspi_id_t qspi,
                                   uint32_t data1, uint32_t data2,
                                   uint32_t data3, uint32_t data4,
                                   uint32_t data5, uint32_t data6,
                                   uint32_t data7, uint32_t data8,
                                   ecos_qspi_cs_t cs)
{
    if (!qspi_id_is_valid(qspi) || !qspi_cs_is_valid(cs))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_write_32x8_cs(
        (hal_qspi_port_t)qspi, data1, data2, data3, data4,
        data5, data6, data7, data8, qspi_hal_cs(cs)
    ));
}

ecos_err_t ecos_qspi_write_32x16_cs(ecos_qspi_id_t qspi,
                                    uint32_t data1, uint32_t data2,
                                    uint32_t data3, uint32_t data4,
                                    uint32_t data5, uint32_t data6,
                                    uint32_t data7, uint32_t data8,
                                    uint32_t data9, uint32_t data10,
                                    uint32_t data11, uint32_t data12,
                                    uint32_t data13, uint32_t data14,
                                    uint32_t data15, uint32_t data16,
                                    ecos_qspi_cs_t cs)
{
    if (!qspi_id_is_valid(qspi) || !qspi_cs_is_valid(cs))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_write_32x16_cs(
        (hal_qspi_port_t)qspi, data1, data2, data3, data4,
        data5, data6, data7, data8, data9, data10, data11, data12,
        data13, data14, data15, data16, qspi_hal_cs(cs)
    ));
}

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
                                    ecos_qspi_cs_t cs)
{
    if (!qspi_id_is_valid(qspi) || !qspi_cs_is_valid(cs))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_write_32x32_cs(
        (hal_qspi_port_t)qspi, data1, data2, data3, data4,
        data5, data6, data7, data8, data9, data10, data11, data12,
        data13, data14, data15, data16, data17, data18, data19, data20,
        data21, data22, data23, data24, data25, data26, data27, data28,
        data29, data30, data31, data32, qspi_hal_cs(cs)
    ));
}

ecos_err_t ecos_qspi_send_cmd(ecos_qspi_id_t qspi,
                              uint8_t command, uint8_t command_bits,
                              uint32_t address, uint8_t address_bits)
{
    if (!qspi_id_is_valid(qspi) ||
        !qspi_validate_phase(command_bits, address_bits))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_send_cmd(
        (hal_qspi_port_t)qspi, command, command_bits, address, address_bits
    ));
}

ecos_err_t ecos_qspi_write(ecos_qspi_id_t qspi,
                           uint8_t command, uint8_t command_bits,
                           uint32_t address, uint8_t address_bits,
                           const void *data, size_t size)
{
    if (!qspi_id_is_valid(qspi) ||
        !qspi_validate_phase(command_bits, address_bits) ||
        !qspi_validate_data(data, size))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_write(
        (hal_qspi_port_t)qspi, command, command_bits, address, address_bits,
        (const uint8_t *)data, (uint16_t)size
    ));
}

ecos_err_t ecos_qspi_read(ecos_qspi_id_t qspi,
                          uint8_t command, uint8_t command_bits,
                          uint32_t address, uint8_t address_bits,
                          uint8_t dummy_cycles, void *data, size_t size)
{
    if (!qspi_id_is_valid(qspi) ||
        !qspi_validate_phase(command_bits, address_bits) ||
        !qspi_validate_data(data, size))
        return ECOS_ERR_INVALID_ARGUMENT;
    return qspi_map_hal_result(hal_qspi_read(
        (hal_qspi_port_t)qspi, command, command_bits, address, address_bits,
        dummy_cycles, (uint8_t *)data, (uint16_t)size
    ));
}
