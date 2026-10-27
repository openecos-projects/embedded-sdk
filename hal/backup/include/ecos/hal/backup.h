#ifndef ECOS_HAL_BACKUP_H
#define ECOS_HAL_BACKUP_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_backup_id_t;
#define HAL_BACKUP_ID_0 0u

ecos_err_t hal_backup_read(hal_backup_id_t id, uint8_t index, uint16_t *value);
ecos_err_t hal_backup_write(hal_backup_id_t id, uint8_t index, uint16_t value);
ecos_err_t hal_backup_reset(hal_backup_id_t id);

#ifdef __cplusplus
}
#endif

#endif
