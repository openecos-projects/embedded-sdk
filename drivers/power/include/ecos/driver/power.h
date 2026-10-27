#ifndef ECOS_DRIVER_POWER_H
#define ECOS_DRIVER_POWER_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_power_id_t;
typedef enum {
    ECOS_POWER_SLEEP_WFI = 0,
    ECOS_POWER_SLEEP_WFE,
    ECOS_POWER_STOP_WFI,
    ECOS_POWER_STOP_WFE,
    ECOS_POWER_STANDBY
} ecos_power_mode_t;
#define ECOS_POWER_DEFAULT ((ecos_power_id_t)0u)

ecos_err_t ecos_power_set_backup_access(ecos_power_id_t id, uint8_t enable);
ecos_err_t ecos_power_set_wakeup_pin(ecos_power_id_t id, uint8_t enable);
ecos_err_t ecos_power_set_pvd(ecos_power_id_t id, uint8_t enable, uint8_t level);
ecos_err_t ecos_power_get_flags(ecos_power_id_t id, uint32_t *flags);
ecos_err_t ecos_power_clear_flags(ecos_power_id_t id, uint32_t flags);
ecos_err_t ecos_power_enter(ecos_power_id_t id, ecos_power_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
