#include "ecos/device/pcf8563.h"

#include <stddef.h>

#define PCF8563_TRY(expression)                    \
    do {                                           \
        ecos_err_t pcf8563_result = (expression);  \
        if (pcf8563_result != ECOS_OK)             \
            return pcf8563_result;                 \
    } while (0)

#define PCF8563_REG_SECOND 0x02u
#define PCF8563_TIME_REG_COUNT 7u

#define PCF8563_SECOND_MASK 0x7Fu
#define PCF8563_MINUTE_MASK 0x7Fu
#define PCF8563_HOUR_MASK 0x3Fu
#define PCF8563_DAY_MASK 0x3Fu
#define PCF8563_WEEKDAY_MASK 0x07u
#define PCF8563_MONTH_MASK 0x1Fu
#define PCF8563_YEAR_MASK 0xFFu

/* The rv32e runtime links without libgcc division helpers; BCD conversion
 * uses repeated subtraction instead of / and %. */
static uint8_t pcf8563_bin_to_bcd(uint8_t value)
{
    uint8_t tens = 0u;

    while (value >= 10u) {
        value = (uint8_t)(value - 10u);
        ++tens;
    }
    return (uint8_t)((tens << 4) | value);
}

static uint8_t pcf8563_bcd_to_bin(uint8_t value, uint8_t mask)
{
    uint8_t tens = (uint8_t)((value & (uint8_t)(mask & 0xF0u)) >> 4);
    uint8_t ones = (uint8_t)(value & (uint8_t)(mask & 0x0Fu));
    uint8_t result = ones;

    while (tens != 0u) {
        result = (uint8_t)(result + 10u);
        --tens;
    }
    return result;
}

static ecos_err_t pcf8563_check_ready(const ecos_pcf8563_t *rtc)
{
    if (rtc == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (rtc->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    return ECOS_OK;
}

ecos_err_t ecos_pcf8563_init(ecos_pcf8563_t *rtc,
                             const ecos_pcf8563_config_t *config)
{
    int probe_result;

    if (rtc == NULL || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    probe_result = ecos_i2c_probe(config->i2c, config->address);
    if (probe_result < 0)
        return (ecos_err_t)probe_result;
    if (probe_result == 0)
        return ECOS_ERR_NOT_FOUND;

    rtc->config = *config;
    rtc->initialized = 1u;
    return ECOS_OK;
}

ecos_err_t ecos_pcf8563_deinit(ecos_pcf8563_t *rtc)
{
    if (rtc == NULL || rtc->initialized == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    rtc->initialized = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_pcf8563_get_time(ecos_pcf8563_t *rtc,
                                 ecos_pcf8563_time_t *time)
{
    uint8_t address = PCF8563_REG_SECOND;
    uint8_t frame[PCF8563_TIME_REG_COUNT];

    PCF8563_TRY(pcf8563_check_ready(rtc));
    if (time == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    PCF8563_TRY(ecos_i2c_write_read(rtc->config.i2c, rtc->config.address,
                                    &address, sizeof(address),
                                    frame, sizeof(frame)));

    time->second = pcf8563_bcd_to_bin(frame[0], PCF8563_SECOND_MASK);
    time->minute = pcf8563_bcd_to_bin(frame[1], PCF8563_MINUTE_MASK);
    time->hour = pcf8563_bcd_to_bin(frame[2], PCF8563_HOUR_MASK);
    time->day = pcf8563_bcd_to_bin(frame[3], PCF8563_DAY_MASK);
    time->weekday = pcf8563_bcd_to_bin(frame[4], PCF8563_WEEKDAY_MASK);
    time->month = pcf8563_bcd_to_bin(frame[5], PCF8563_MONTH_MASK);
    time->year = pcf8563_bcd_to_bin(frame[6], PCF8563_YEAR_MASK);
    return ECOS_OK;
}

ecos_err_t ecos_pcf8563_set_time(ecos_pcf8563_t *rtc,
                                 const ecos_pcf8563_time_t *time)
{
    uint8_t frame[1u + PCF8563_TIME_REG_COUNT];

    PCF8563_TRY(pcf8563_check_ready(rtc));
    if (time == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (time->second > 59u || time->minute > 59u || time->hour > 23u ||
        time->day == 0u || time->day > 31u || time->weekday > 6u ||
        time->month == 0u || time->month > 12u || time->year > 99u)
        return ECOS_ERR_INVALID_ARGUMENT;

    frame[0] = PCF8563_REG_SECOND;
    frame[1] = pcf8563_bin_to_bcd(time->second);
    frame[2] = pcf8563_bin_to_bcd(time->minute);
    frame[3] = pcf8563_bin_to_bcd(time->hour);
    frame[4] = pcf8563_bin_to_bcd(time->day);
    frame[5] = pcf8563_bin_to_bcd(time->weekday);
    frame[6] = pcf8563_bin_to_bcd(time->month);
    frame[7] = pcf8563_bin_to_bcd(time->year);
    return ecos_i2c_write(rtc->config.i2c, rtc->config.address,
                          frame, sizeof(frame));
}
