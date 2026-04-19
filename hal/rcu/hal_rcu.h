#ifndef __HAL_RCU_H__
#define __HAL_RCU_H__

#include <stdint.h>

void hal_rcu_set_ctrl(uint32_t val);
void hal_rcu_set_rdiv(uint32_t val);
uint32_t hal_rcu_get_stat(void);

#endif
