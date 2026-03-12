/**
 * @file timer_driver.h
 * @brief StarrySky C2 Timer driver header
 */

#ifndef TIMER_DRIVER_H__
#define TIMER_DRIVER_H__

#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C2 Timer driver
 * @return Status code
 */
status_t starrysky_c2_timer_init(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_DRIVER_H__ */