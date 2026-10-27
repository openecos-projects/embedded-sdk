#ifndef ECOS_DRIVER_WWDG_H
#define ECOS_DRIVER_WWDG_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_wwdg_id_t;
typedef struct {
    uint32_t prescaler;
    uint8_t window;
    uint8_t counter;
    uint8_t interrupt_enable;
} ecos_wwdg_config_t;
#define ECOS_WWDG_DEFAULT ((ecos_wwdg_id_t)0u)
#define ECOS_WWDG_CONFIG_DEFAULT { 8u, 0x50u, 0x7Fu, 0u }

ecos_err_t ecos_wwdg_init(ecos_wwdg_id_t id, const ecos_wwdg_config_t *config);
ecos_err_t ecos_wwdg_deinit(ecos_wwdg_id_t id);
ecos_err_t ecos_wwdg_feed(ecos_wwdg_id_t id, uint8_t counter);
ecos_err_t ecos_wwdg_get_status(ecos_wwdg_id_t id, uint32_t *status);
ecos_err_t ecos_wwdg_clear_status(ecos_wwdg_id_t id);

#ifdef __cplusplus
}
#endif

#endif
