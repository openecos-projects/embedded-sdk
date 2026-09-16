#ifndef ECOS_DEVICE_AHT20_H
#define ECOS_DEVICE_AHT20_H

#include "ecos/driver/i2c.h"
#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AHT20 7-bit I2C address. */
#define ECOS_AHT20_I2C_ADDRESS 0x38u

typedef struct {
    ecos_i2c_id_t i2c;
    uint8_t address;
} ecos_aht20_config_t;

typedef struct {
    ecos_aht20_config_t config;
    uint8_t initialized;
} ecos_aht20_t;

/* Fixed-point readings to avoid pulling in floating-point formatting:
 * temperature_x100 is degrees Celsius times 100 (may be negative),
 * humidity_x100 is relative humidity in percent times 100. */
typedef struct {
    int32_t temperature_x100;
    uint32_t humidity_x100;
} ecos_aht20_data_t;

#define ECOS_AHT20_CONFIG_DEFAULT \
    { ECOS_I2C_DEFAULT, ECOS_AHT20_I2C_ADDRESS }

/* The I2C controller must be initialized by the caller before ecos_aht20_init. */
ecos_err_t ecos_aht20_init(ecos_aht20_t *sensor,
                           const ecos_aht20_config_t *config);
ecos_err_t ecos_aht20_deinit(ecos_aht20_t *sensor);

/* Triggers one measurement and returns the converted readings. */
ecos_err_t ecos_aht20_read(ecos_aht20_t *sensor, ecos_aht20_data_t *data);

#ifdef __cplusplus
}
#endif

#endif
