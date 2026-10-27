#ifndef ECOS_HAL_WWDG_H
#define ECOS_HAL_WWDG_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_wwdg_id_t;
#define HAL_WWDG_ID_0 0u

typedef struct {
    uint32_t prescaler;       /* 1, 2, 4 or 8 */
    uint8_t window;           /* 0x40..0x7f */
    uint8_t counter;          /* 0x40..0x7f, counter > window */
    uint8_t interrupt_enable;
} hal_wwdg_config_t;

ecos_err_t hal_wwdg_init(hal_wwdg_id_t id, const hal_wwdg_config_t *config);
ecos_err_t hal_wwdg_deinit(hal_wwdg_id_t id);
ecos_err_t hal_wwdg_feed(hal_wwdg_id_t id, uint8_t counter);
ecos_err_t hal_wwdg_get_status(hal_wwdg_id_t id, uint32_t *status);
ecos_err_t hal_wwdg_clear_status(hal_wwdg_id_t id);

#ifdef __cplusplus
}
#endif

#endif
