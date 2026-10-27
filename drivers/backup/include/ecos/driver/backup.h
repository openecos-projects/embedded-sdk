#ifndef ECOS_DRIVER_BACKUP_H
#define ECOS_DRIVER_BACKUP_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_backup_id_t;
#define ECOS_BACKUP_DEFAULT ((ecos_backup_id_t)0u)

ecos_err_t ecos_backup_read(ecos_backup_id_t id, uint8_t index, uint16_t *value);
ecos_err_t ecos_backup_write(ecos_backup_id_t id, uint8_t index, uint16_t value);
ecos_err_t ecos_backup_reset(ecos_backup_id_t id);

#ifdef __cplusplus
}
#endif

#endif
