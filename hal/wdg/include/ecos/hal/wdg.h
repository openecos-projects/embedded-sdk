#ifndef ECOS_HAL_WDG_H
#define ECOS_HAL_WDG_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_WDG_ID_0 = 0,
    HAL_WDG_ID_COUNT
} hal_wdg_id_t;

typedef struct {
    uint32_t prescaler;
    uint32_t compare;
    uint32_t control;
} hal_wdg_config_t;

ecos_err_t hal_wdg_init(hal_wdg_id_t id, const hal_wdg_config_t *config);
ecos_err_t hal_wdg_deinit(hal_wdg_id_t id);
ecos_err_t hal_wdg_feed(hal_wdg_id_t id);
ecos_err_t hal_wdg_get_status(hal_wdg_id_t id, uint32_t *status);

#ifdef __cplusplus
}
#endif

#endif
