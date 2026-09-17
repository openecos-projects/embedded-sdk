#ifndef ECOS_DRIVER_RNG_H
#define ECOS_DRIVER_RNG_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_RNG_ID_0 = 0,
    ECOS_RNG_ID_COUNT
} ecos_rng_id_t;

#define ECOS_RNG_DEFAULT ((ecos_rng_id_t)ECOS_RNG_ID_0)

typedef struct {
    uint32_t seed;
} ecos_rng_config_t;

#define ECOS_RNG_CONFIG_DEFAULT \
    { 1u }

ecos_err_t ecos_rng_init(ecos_rng_id_t id, const ecos_rng_config_t *config);
ecos_err_t ecos_rng_deinit(ecos_rng_id_t id);
ecos_err_t ecos_rng_read(ecos_rng_id_t id, uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif
