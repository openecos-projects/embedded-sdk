#ifndef ECOS_HAL_RNG_H
#define ECOS_HAL_RNG_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_RNG_ID_0 = 0,
    HAL_RNG_ID_COUNT
} hal_rng_id_t;

typedef struct {
    uint32_t seed;
} hal_rng_config_t;

ecos_err_t hal_rng_init(hal_rng_id_t id, const hal_rng_config_t *config);
ecos_err_t hal_rng_deinit(hal_rng_id_t id);
ecos_err_t hal_rng_read(hal_rng_id_t id, uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif
