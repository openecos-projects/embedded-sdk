#ifndef ECOS_HAL_PS2_H
#define ECOS_HAL_PS2_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_PS2_ID_0 = 0,
    HAL_PS2_ID_COUNT
} hal_ps2_id_t;

ecos_err_t hal_ps2_init(hal_ps2_id_t id);
ecos_err_t hal_ps2_deinit(hal_ps2_id_t id);
ecos_err_t hal_ps2_read(hal_ps2_id_t id, uint32_t *scan_code);

#ifdef __cplusplus
}
#endif

#endif
