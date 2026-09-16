#include "ecos/device/aht20.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define AHT20_TRY(expression)                    \
    do {                                         \
        ecos_err_t aht20_result = (expression);  \
        if (aht20_result != ECOS_OK)             \
            return aht20_result;                 \
    } while (0)

/* AHT20 command words. */
#define AHT20_CMD_CALIBRATE 0xBEu
#define AHT20_CMD_TRIGGER 0xACu
#define AHT20_CMD_SOFT_RESET 0xBAu

#define AHT20_STATUS_BUSY 0x80u
#define AHT20_STATUS_CALIBRATED 0x08u

#define AHT20_POWER_ON_DELAY_MS 40u
#define AHT20_CALIBRATE_DELAY_MS 10u
#define AHT20_MEASURE_DELAY_MS 80u
#define AHT20_BUSY_POLL_COUNT 20u
#define AHT20_BUSY_POLL_INTERVAL_MS 10u

#define AHT20_FRAME_SIZE 7u /* status + 5 data bytes + CRC */

/* CRC-8, polynomial 0x31, initial value 0xFF, over the first 6 frame bytes. */
static uint8_t aht20_crc8(const uint8_t *data, uint8_t length)
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

static ecos_err_t aht20_read_status(ecos_aht20_t *sensor, uint8_t *status)
{
    return ecos_i2c_read(sensor->config.i2c, sensor->config.address,
                         status, 1u);
}

static ecos_err_t aht20_wait_idle(ecos_aht20_t *sensor)
{
    uint8_t status = 0u;
    uint8_t attempt;

    for (attempt = 0u; attempt < AHT20_BUSY_POLL_COUNT; ++attempt) {
        AHT20_TRY(aht20_read_status(sensor, &status));
        if ((status & AHT20_STATUS_BUSY) == 0u)
            return ECOS_OK;
        AHT20_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT,
                                      AHT20_BUSY_POLL_INTERVAL_MS));
    }
    return ECOS_ERR_TIMEOUT;
}

ecos_err_t ecos_aht20_init(ecos_aht20_t *sensor,
                           const ecos_aht20_config_t *config)
{
    static const uint8_t calibrate[] = { AHT20_CMD_CALIBRATE, 0x08u, 0x00u };
    uint8_t status = 0u;

    if (sensor == NULL || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    sensor->config = *config;
    sensor->initialized = 0u;

    AHT20_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT,
                                  AHT20_POWER_ON_DELAY_MS));
    AHT20_TRY(ecos_i2c_write(config->i2c, config->address,
                             calibrate, sizeof(calibrate)));
    AHT20_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT,
                                  AHT20_CALIBRATE_DELAY_MS));
    AHT20_TRY(aht20_wait_idle(sensor));
    AHT20_TRY(aht20_read_status(sensor, &status));
    if ((status & AHT20_STATUS_CALIBRATED) == 0u)
        return ECOS_ERR_IO;

    sensor->initialized = 1u;
    return ECOS_OK;
}

ecos_err_t ecos_aht20_deinit(ecos_aht20_t *sensor)
{
    if (sensor == NULL || sensor->initialized == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    sensor->initialized = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_aht20_read(ecos_aht20_t *sensor, ecos_aht20_data_t *data)
{
    static const uint8_t trigger[] = { AHT20_CMD_TRIGGER, 0x33u, 0x00u };
    uint8_t frame[AHT20_FRAME_SIZE];
    uint32_t humidity_raw;
    uint32_t temperature_raw;

    if (sensor == NULL || data == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (sensor->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    AHT20_TRY(ecos_i2c_write(sensor->config.i2c, sensor->config.address,
                             trigger, sizeof(trigger)));
    AHT20_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT,
                                  AHT20_MEASURE_DELAY_MS));
    AHT20_TRY(aht20_wait_idle(sensor));
    AHT20_TRY(ecos_i2c_read(sensor->config.i2c, sensor->config.address,
                            frame, sizeof(frame)));
    if (aht20_crc8(frame, AHT20_FRAME_SIZE - 1u) != frame[AHT20_FRAME_SIZE - 1u])
        return ECOS_ERR_IO;

    /* frame[0] is the status byte; humidity occupies bits [27:8] of the
     * remaining 40-bit word, temperature bits [19:0]. */
    humidity_raw = ((uint32_t)frame[1] << 12) |
                   ((uint32_t)frame[2] << 4) |
                   ((uint32_t)frame[3] >> 4);
    temperature_raw = (((uint32_t)frame[3] & 0x0Fu) << 16) |
                      ((uint32_t)frame[4] << 8) |
                      (uint32_t)frame[5];

    data->humidity_x100 =
        (uint32_t)(((uint64_t)humidity_raw * 10000u) / 1048576u);
    data->temperature_x100 =
        (int32_t)(((uint64_t)temperature_raw * 20000u) / 1048576u) - 5000;
    return ECOS_OK;
}
