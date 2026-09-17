#ifndef ECOS_HAL_RCU_H
#define ECOS_HAL_RCU_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_RCU_ID_0 = 0,
    HAL_RCU_ID_COUNT
} hal_rcu_id_t;

typedef struct {
    uint32_t clock_divider;   /* 分频比，硬件写入 RDIV = clock_divider - 1 */
    uint32_t control;         /* CTRL 原始位，位定义为 SoC 私有 */
} hal_rcu_config_t;

ecos_err_t hal_rcu_init(hal_rcu_id_t id, const hal_rcu_config_t *config);
ecos_err_t hal_rcu_deinit(hal_rcu_id_t id);
ecos_err_t hal_rcu_get_status(hal_rcu_id_t id, uint32_t *status);

#ifdef __cplusplus
}
#endif

#endif
