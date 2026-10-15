#ifndef ECOS_DRIVER_PS2_H
#define ECOS_DRIVER_PS2_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_PS2_ID_0 = 0,
    ECOS_PS2_ID_COUNT
} ecos_ps2_id_t;

#define ECOS_PS2_DEFAULT ((ecos_ps2_id_t)ECOS_PS2_ID_0)

ecos_err_t ecos_ps2_init(ecos_ps2_id_t id);
ecos_err_t ecos_ps2_deinit(ecos_ps2_id_t id);

/* A zero scan code means no key data, matching the legacy register interface. */
ecos_err_t ecos_ps2_read(ecos_ps2_id_t id, uint32_t *scan_code);

#ifdef __cplusplus
}
#endif

#endif
