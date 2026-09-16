#ifndef ECOS_HAL_I2C_H
#define ECOS_HAL_I2C_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_i2c_id_t;

typedef struct {
    uint32_t clock_divider;
} hal_i2c_config_t;

int hal_i2c_get_instance_count(void);
ecos_err_t hal_i2c_init(hal_i2c_id_t i2c,
                        const hal_i2c_config_t *config);
ecos_err_t hal_i2c_deinit(hal_i2c_id_t i2c);
int hal_i2c_probe(hal_i2c_id_t i2c, uint8_t address);

/* Execute a complete transaction against a 7-bit slave address. */
ecos_err_t hal_i2c_write(hal_i2c_id_t i2c,
                         uint8_t address,
                         const uint8_t *data,
                         size_t size);
ecos_err_t hal_i2c_read(hal_i2c_id_t i2c,
                        uint8_t address,
                        uint8_t *data,
                        size_t size);
/* Write bytes, issue a repeated START, then read bytes. */
ecos_err_t hal_i2c_write_read(hal_i2c_id_t i2c,
                              uint8_t address,
                              const uint8_t *write_data,
                              size_t write_size,
                              uint8_t *read_data,
                              size_t read_size);

#ifdef __cplusplus
}
#endif

#endif
