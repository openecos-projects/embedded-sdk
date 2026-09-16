#ifndef ECOS_DEVICE_PCF8563_H
#define ECOS_DEVICE_PCF8563_H

#include "ecos/driver/i2c.h"
#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PCF8563 7-bit I2C address. */
#define ECOS_PCF8563_I2C_ADDRESS 0x51u

typedef struct {
    ecos_i2c_id_t i2c;
    uint8_t address;
} ecos_pcf8563_config_t;

typedef struct {
    ecos_pcf8563_config_t config;
    uint8_t initialized;
} ecos_pcf8563_t;

typedef struct {
    uint8_t second;  /* 0-59 */
    uint8_t minute;  /* 0-59 */
    uint8_t hour;    /* 0-23 */
    uint8_t day;     /* 1-31 */
    uint8_t weekday; /* 0-6 */
    uint8_t month;   /* 1-12 */
    uint8_t year;    /* 0-99, offset from 2000 */
} ecos_pcf8563_time_t;

#define ECOS_PCF8563_CONFIG_DEFAULT \
    { ECOS_I2C_DEFAULT, ECOS_PCF8563_I2C_ADDRESS }

/* The I2C controller must be initialized by the caller; init only probes
 * the device and does not touch the RTC registers. */
ecos_err_t ecos_pcf8563_init(ecos_pcf8563_t *rtc,
                             const ecos_pcf8563_config_t *config);
ecos_err_t ecos_pcf8563_deinit(ecos_pcf8563_t *rtc);

ecos_err_t ecos_pcf8563_get_time(ecos_pcf8563_t *rtc,
                                 ecos_pcf8563_time_t *time);
ecos_err_t ecos_pcf8563_set_time(ecos_pcf8563_t *rtc,
                                 const ecos_pcf8563_time_t *time);

#ifdef __cplusplus
}
#endif

#endif
