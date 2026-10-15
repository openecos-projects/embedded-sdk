#include "hal_rcu.h"
#include "board.h"

void hal_rcu_set_ctrl(uint32_t val) {
    REG_RCU_0_CTRL = val;
}

void hal_rcu_set_rdiv(uint32_t val) {
    REG_RCU_0_RDIV = val;
}

uint32_t hal_rcu_get_stat(void) {
    return REG_RCU_0_STAT;
}
