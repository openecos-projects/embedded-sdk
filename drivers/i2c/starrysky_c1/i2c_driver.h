/**
 * @file i2c_driver.h
 * @brief StarrySky C1 I2C driver header
 */

#ifndef I2C_DRIVER_H__
#define I2C_DRIVER_H__

#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C1 I2C driver
 * @return Status code
 */
status_t starrysky_c1_i2c_init(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_DRIVER_H__ */