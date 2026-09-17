#ifndef ECOS_DRIVER_ARCHINFO_H
#define ECOS_DRIVER_ARCHINFO_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_ARCHINFO_ID_0 = 0,
    ECOS_ARCHINFO_ID_COUNT
} ecos_archinfo_id_t;

#define ECOS_ARCHINFO_DEFAULT ((ecos_archinfo_id_t)ECOS_ARCHINFO_ID_0)

/* Archinfo 是只读的芯片标识寄存器组，无需初始化。 */
ecos_err_t ecos_archinfo_get_system_id(ecos_archinfo_id_t id, uint32_t *value);
ecos_err_t ecos_archinfo_get_chip_id(ecos_archinfo_id_t id, uint64_t *value);

#ifdef __cplusplus
}
#endif

#endif
