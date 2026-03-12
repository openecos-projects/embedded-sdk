/**
 * @file gpio_driver.h
 * @brief StarrySky C2 GPIO driver header
 */

#ifndef GPIO_DRIVER_H__
#define GPIO_DRIVER_H__

#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C2 GPIO driver
 * @return Status code
 */
status_t starrysky_c2_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_DRIVER_H__ */