/**
 * @file qspi_driver.h
 * @brief StarrySky C2 QSPI driver header
 */

#ifndef QSPI_DRIVER_H__
#define QSPI_DRIVER_H__

#include "qspi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C2 QSPI driver
 * @return Status code
 */
status_t starrysky_c2_qspi_init(void);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_DRIVER_H__ */