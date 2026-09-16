#include "ecos/device/sgp30.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define SGP30_TRY(expression)                    \
    do {                                         \
        ecos_err_t sgp30_result = (expression);  \
        if (sgp30_result != ECOS_OK)             \
            return sgp30_result;                 \
    } while (0)

#define SGP30_CMD_INIT_AIR_QUALITY 0x2003u
#define SGP30_CMD_MEASURE_AIR_QUALITY 0x2008u
#define SGP30_CMD_GET_BASELINE 0x2015u
#define SGP30_CMD_SET_BASELINE 0x201Eu
#define SGP30_CMD_GET_SERIAL_ID 0x3682u

#define SGP30_GENERIC_DELAY_MS 10u
#define SGP30_MEASURE_DELAY_MS 12u

/* Sensirion CRC-8, polynomial 0x31, initial value 0xFF. */
static uint8_t sgp30_crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0xFFu;
    uint8_t byte;
    uint8_t bit;

    for (byte = 0u; byte < length; ++byte) {
        crc ^= data[byte];
        for (bit = 0u; bit < 8u; ++bit) {
            if ((crc & 0x80u) != 0u)
                crc = (uint8_t)((crc << 1) ^ 0x31u);
            else
                crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static ecos_err_t sgp30_write_command(ecos_sgp30_t *sensor, uint16_t command)
{
    uint8_t buffer[2] = {
        (uint8_t)(command >> 8), (uint8_t)(command & 0xFFu)
    };

    return ecos_i2c_write(sensor->config.i2c, sensor->config.address,
                          buffer, sizeof(buffer));
}

/* Sends the command, waits for the conversion and reads the response. */
static ecos_err_t sgp30_read_command(ecos_sgp30_t *sensor,
                                     uint16_t command,
                                     uint32_t delay_ms,
                                     uint8_t *data,
                                     size_t size)
{
    uint8_t buffer[2] = {
        (uint8_t)(command >> 8), (uint8_t)(command & 0xFFu)
    };

    SGP30_TRY(ecos_i2c_write(sensor->config.i2c, sensor->config.address,
                             buffer, sizeof(buffer)));
    SGP30_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, delay_ms));
    return ecos_i2c_read(sensor->config.i2c, sensor->config.address,
                         data, size);
}

static ecos_err_t sgp30_check_ready(const ecos_sgp30_t *sensor)
{
    if (sensor == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (sensor->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    return ECOS_OK;
}

ecos_err_t ecos_sgp30_init(ecos_sgp30_t *sensor,
                           const ecos_sgp30_config_t *config)
{
    int probe_result;

    if (sensor == NULL || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    probe_result = ecos_i2c_probe(config->i2c, config->address);
    if (probe_result < 0)
        return (ecos_err_t)probe_result;
    if (probe_result == 0)
        return ECOS_ERR_NOT_FOUND;

    sensor->config = *config;
    sensor->initialized = 1u;
    return sgp30_write_command(sensor, SGP30_CMD_INIT_AIR_QUALITY);
}

ecos_err_t ecos_sgp30_deinit(ecos_sgp30_t *sensor)
{
    if (sensor == NULL || sensor->initialized == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    sensor->initialized = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_sgp30_read_serial_id(ecos_sgp30_t *sensor,
                                     uint64_t *serial_id)
{
    uint8_t frame[9];
    uint8_t word;

    SGP30_TRY(sgp30_check_ready(sensor));
    if (serial_id == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    SGP30_TRY(sgp30_read_command(sensor, SGP30_CMD_GET_SERIAL_ID,
                                 SGP30_GENERIC_DELAY_MS,
                                 frame, sizeof(frame)));
    *serial_id = 0u;
    for (word = 0u; word < 3u; ++word) {
        uint8_t high = frame[word * 3u];
        uint8_t low = frame[word * 3u + 1u];

        if (sgp30_crc8(&frame[word * 3u], 2u) != frame[word * 3u + 2u])
            return ECOS_ERR_IO;
        *serial_id = (*serial_id << 16) | ((uint64_t)high << 8) | low;
    }
    return ECOS_OK;
}

ecos_err_t ecos_sgp30_measure_air_quality(ecos_sgp30_t *sensor,
                                          ecos_sgp30_air_quality_t *air_quality)
{
    uint8_t frame[6];

    SGP30_TRY(sgp30_check_ready(sensor));
    if (air_quality == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    SGP30_TRY(sgp30_read_command(sensor, SGP30_CMD_MEASURE_AIR_QUALITY,
                                 SGP30_MEASURE_DELAY_MS,
                                 frame, sizeof(frame)));
    if (sgp30_crc8(&frame[0], 2u) != frame[2] ||
        sgp30_crc8(&frame[3], 2u) != frame[5])
        return ECOS_ERR_IO;

    air_quality->co2_eq_ppm = ((uint16_t)frame[0] << 8) | frame[1];
    air_quality->tvoc_ppb = ((uint16_t)frame[3] << 8) | frame[4];
    return ECOS_OK;
}

ecos_err_t ecos_sgp30_get_baseline(ecos_sgp30_t *sensor,
                                   ecos_sgp30_baseline_t *baseline)
{
    uint8_t frame[6];

    SGP30_TRY(sgp30_check_ready(sensor));
    if (baseline == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    SGP30_TRY(sgp30_read_command(sensor, SGP30_CMD_GET_BASELINE,
                                 SGP30_GENERIC_DELAY_MS,
                                 frame, sizeof(frame)));
    if (sgp30_crc8(&frame[0], 2u) != frame[2] ||
        sgp30_crc8(&frame[3], 2u) != frame[5])
        return ECOS_ERR_IO;

    baseline->co2_eq_baseline = ((uint16_t)frame[0] << 8) | frame[1];
    baseline->tvoc_baseline = ((uint16_t)frame[3] << 8) | frame[4];
    return ECOS_OK;
}

ecos_err_t ecos_sgp30_set_baseline(ecos_sgp30_t *sensor,
                                   const ecos_sgp30_baseline_t *baseline)
{
    uint8_t buffer[8];

    SGP30_TRY(sgp30_check_ready(sensor));
    if (baseline == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    buffer[0] = (uint8_t)(SGP30_CMD_SET_BASELINE >> 8);
    buffer[1] = (uint8_t)(SGP30_CMD_SET_BASELINE & 0xFFu);
    buffer[2] = (uint8_t)(baseline->tvoc_baseline >> 8);
    buffer[3] = (uint8_t)(baseline->tvoc_baseline & 0xFFu);
    buffer[4] = sgp30_crc8(&buffer[2], 2u);
    buffer[5] = (uint8_t)(baseline->co2_eq_baseline >> 8);
    buffer[6] = (uint8_t)(baseline->co2_eq_baseline & 0xFFu);
    buffer[7] = sgp30_crc8(&buffer[5], 2u);
    return ecos_i2c_write(sensor->config.i2c, sensor->config.address,
                          buffer, sizeof(buffer));
}
