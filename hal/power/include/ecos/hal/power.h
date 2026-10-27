#ifndef ECOS_HAL_POWER_H
#define ECOS_HAL_POWER_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_POWER_ID_0 = 0
} hal_power_id_t;

typedef enum {
    HAL_POWER_SLEEP_WFI = 0,
    HAL_POWER_SLEEP_WFE,
    HAL_POWER_STOP_WFI,
    HAL_POWER_STOP_WFE,
    HAL_POWER_STANDBY
} hal_power_mode_t;

ecos_err_t hal_power_set_backup_access(hal_power_id_t id, uint8_t enable);
ecos_err_t hal_power_set_wakeup_pin(hal_power_id_t id, uint8_t enable);
ecos_err_t hal_power_set_pvd(hal_power_id_t id, uint8_t enable,
                             uint8_t level);
ecos_err_t hal_power_get_flags(hal_power_id_t id, uint32_t *flags);
ecos_err_t hal_power_clear_flags(hal_power_id_t id, uint32_t flags);
ecos_err_t hal_power_enter(hal_power_id_t id, hal_power_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
