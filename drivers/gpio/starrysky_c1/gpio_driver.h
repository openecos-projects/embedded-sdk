/**
 * @file gpio_driver.h
 * @brief StarrySky C1 GPIO driver header
 */

#ifndef GPIO_DRIVER_H__
#define GPIO_DRIVER_H__

#include "hal_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C1 GPIO driver
 * @return HAL status code
 */
hal_status_t starrysky_c1_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_DRIVER_H__ */