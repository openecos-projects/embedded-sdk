#ifndef ECOS_DRIVER_I2C_H
#define ECOS_DRIVER_I2C_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_i2c_id_t;

#define ECOS_I2C_DEFAULT ((ecos_i2c_id_t)0u)

typedef struct {
    uint32_t clock_divider;
} ecos_i2c_config_t;

#define ECOS_I2C_CONFIG_DEFAULT \
    { 100u }

/* Returns the number of I2C controller instances provided by the Target. */
int ecos_i2c_get_instance_count(void);

ecos_err_t ecos_i2c_init(ecos_i2c_id_t i2c,
                         const ecos_i2c_config_t *config);
ecos_err_t ecos_i2c_deinit(ecos_i2c_id_t i2c);

/* Returns 1 for ACK, 0 for NACK, or a negative error code. */
int ecos_i2c_probe(ecos_i2c_id_t i2c, uint8_t address);

#ifdef __cplusplus
}
#endif

#endif
