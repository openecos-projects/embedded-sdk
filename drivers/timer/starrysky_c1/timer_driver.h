/**
 * @file timer_driver.h
 * @brief StarrySky C1 Timer driver header
 */

#ifndef TIMER_DRIVER_H__
#define TIMER_DRIVER_H__

#include "hal_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C1 Timer driver
 * @return HAL status code
 */
hal_status_t starrysky_c1_timer_init(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_DRIVER_H__ */