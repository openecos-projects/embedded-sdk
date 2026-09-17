#ifndef ECOS_HAL_ARCHINFO_H
#define ECOS_HAL_ARCHINFO_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_ARCHINFO_ID_0 = 0,
    HAL_ARCHINFO_ID_COUNT
} hal_archinfo_id_t;

/* Archinfo 是只读的芯片标识寄存器组，没有 init/deinit 生命周期。 */

ecos_err_t hal_archinfo_get_system_id(hal_archinfo_id_t id, uint32_t *value);
ecos_err_t hal_archinfo_get_chip_id(hal_archinfo_id_t id, uint64_t *value);

#ifdef __cplusplus
}
#endif

#endif
