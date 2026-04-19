#ifndef __HAL_RNG_H__
#define __HAL_RNG_H__

#include <stdint.h>

void hal_rng_set_ctrl(uint32_t val);
void hal_rng_set_seed(uint32_t val);
uint32_t hal_rng_get_val(void);

#endif
