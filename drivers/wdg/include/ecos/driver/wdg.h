#ifndef ECOS_DRIVER_WDG_H
#define ECOS_DRIVER_WDG_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_WDG_ID_0 = 0,
    ECOS_WDG_ID_COUNT
} ecos_wdg_id_t;

#define ECOS_WDG_DEFAULT ((ecos_wdg_id_t)ECOS_WDG_ID_0)

/* Prescaler and compare are register values plus one. Control is the raw SoC
 * watchdog control word; the legacy reset example uses 0x5. */
typedef struct {
    uint32_t prescaler;
    uint32_t compare;
    uint32_t control;
} ecos_wdg_config_t;

#define ECOS_WDG_CONFIG_DEFAULT \
    { 100u, 500u, 0x5u }

ecos_err_t ecos_wdg_init(ecos_wdg_id_t id, const ecos_wdg_config_t *config);
ecos_err_t ecos_wdg_deinit(ecos_wdg_id_t id);
ecos_err_t ecos_wdg_feed(ecos_wdg_id_t id);
ecos_err_t ecos_wdg_get_status(ecos_wdg_id_t id, uint32_t *status);

#ifdef __cplusplus
}
#endif

#endif
