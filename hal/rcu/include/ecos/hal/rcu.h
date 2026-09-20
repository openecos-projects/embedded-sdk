#ifndef ECOS_HAL_RCU_H
#define ECOS_HAL_RCU_H

#include "ecos/error.h"

#include <stdbool.h>
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

typedef enum {
    HAL_RCU_PLL_SELECT_X1 = 0,
    HAL_RCU_PLL_SELECT_X2 = 1
} hal_rcu_pll_select_t;

typedef enum {
    HAL_RCU_PLL_OD_1 = 0,
    HAL_RCU_PLL_OD_2 = 1,
    HAL_RCU_PLL_OD_4 = 2,
    HAL_RCU_PLL_OD_8 = 3
} hal_rcu_pll_od_t;

typedef enum {
    HAL_RCU_CLOCK_PLL = 0,
    HAL_RCU_CLOCK_CPU,
    HAL_RCU_CLOCK_AXI,
    HAL_RCU_CLOCK_AHB,
    HAL_RCU_CLOCK_APB
} hal_rcu_clock_domain_t;

typedef struct {
    uint8_t  n;              /* PLL 反馈分频比 N[7:0]，>=1 */
    hal_rcu_pll_select_t select;
    hal_rcu_pll_od_t     od;
    uint8_t  cpu_div, axi_div, ahb_div;  /* 写入 CLK_DIV 的分频字节 */
    bool     keep_apb;       /* true = APB 不切 PLL，保持原时钟 */
    uint32_t settle_ticks;   /* PLL 稳定等待（mtime ticks），0 = 用实现默认值 */
} hal_rcu_pll_config_t;

/* 无 PLL 的 SoC 返回 ECOS_ERR_UNSUPPORTED。 */
ecos_err_t hal_rcu_pll_apply(hal_rcu_id_t id, const hal_rcu_pll_config_t *config);
ecos_err_t hal_rcu_pll_get_freq(hal_rcu_id_t id, hal_rcu_clock_domain_t domain,
                                uint32_t *freq_hz);

#ifdef __cplusplus
}
#endif

#endif
