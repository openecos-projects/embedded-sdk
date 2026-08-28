#include "hal_rng.h"
#include "ysyx_2512_soc.h"

void hal_rng_set_ctrl(uint32_t val) {
    REG_RNG_0_CTRL = val;
}

void hal_rng_set_seed(uint32_t val) {
    REG_RNG_0_SEED = val;
}

uint32_t hal_rng_get_val(void) {
    return REG_RNG_0_VAL;
}
