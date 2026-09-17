#ifndef ECOS_DRIVER_RCU_H
#define ECOS_DRIVER_RCU_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_RCU_ID_0 = 0,
    ECOS_RCU_ID_COUNT
} ecos_rcu_id_t;

#define ECOS_RCU_DEFAULT ((ecos_rcu_id_t)ECOS_RCU_ID_0)

/* control 为 CTRL 寄存器原始位，位定义是 SoC 私有时钟/复位域配置。 */
typedef struct {
    uint32_t clock_divider;
    uint32_t control;
} ecos_rcu_config_t;

#define ECOS_RCU_CONFIG_DEFAULT \
    { 1u, 0u }

ecos_err_t ecos_rcu_init(ecos_rcu_id_t id, const ecos_rcu_config_t *config);
ecos_err_t ecos_rcu_deinit(ecos_rcu_id_t id);
ecos_err_t ecos_rcu_get_status(ecos_rcu_id_t id, uint32_t *status);

#ifdef __cplusplus
}
#endif

#endif
