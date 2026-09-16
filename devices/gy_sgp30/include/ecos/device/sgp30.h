#ifndef ECOS_DEVICE_SGP30_H
#define ECOS_DEVICE_SGP30_H

#include "ecos/driver/i2c.h"
#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SGP30 7-bit I2C address. */
#define ECOS_SGP30_I2C_ADDRESS 0x58u

typedef struct {
    ecos_i2c_id_t i2c;
    uint8_t address;
} ecos_sgp30_config_t;

typedef struct {
    ecos_sgp30_config_t config;
    uint8_t initialized;
} ecos_sgp30_t;

typedef struct {
    uint16_t co2_eq_ppm;
    uint16_t tvoc_ppb;
} ecos_sgp30_air_quality_t;

typedef struct {
    uint16_t co2_eq_baseline;
    uint16_t tvoc_baseline;
} ecos_sgp30_baseline_t;

#define ECOS_SGP30_CONFIG_DEFAULT \
    { ECOS_I2C_DEFAULT, ECOS_SGP30_I2C_ADDRESS }

/* The I2C controller must be initialized by the caller. init probes the
 * device and starts the IAQ measurement (SGP30 returns default values for
 * the first 15 s of operation). */
ecos_err_t ecos_sgp30_init(ecos_sgp30_t *sensor,
                           const ecos_sgp30_config_t *config);
ecos_err_t ecos_sgp30_deinit(ecos_sgp30_t *sensor);

ecos_err_t ecos_sgp30_read_serial_id(ecos_sgp30_t *sensor,
                                     uint64_t *serial_id);
ecos_err_t ecos_sgp30_measure_air_quality(ecos_sgp30_t *sensor,
                                          ecos_sgp30_air_quality_t *air_quality);
ecos_err_t ecos_sgp30_get_baseline(ecos_sgp30_t *sensor,
                                   ecos_sgp30_baseline_t *baseline);
ecos_err_t ecos_sgp30_set_baseline(ecos_sgp30_t *sensor,
                                   const ecos_sgp30_baseline_t *baseline);

#ifdef __cplusplus
}
#endif

#endif
