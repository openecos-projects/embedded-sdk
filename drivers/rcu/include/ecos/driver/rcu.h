#ifndef ECOS_DRIVER_RCU_H
#define ECOS_DRIVER_RCU_H

#include "ecos/error.h"

#include <stdbool.h>
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

typedef enum {
    ECOS_RCU_PLL_SELECT_X1 = 0,
    ECOS_RCU_PLL_SELECT_X2 = 1
} ecos_rcu_pll_select_t;

typedef enum {
    ECOS_RCU_PLL_OD_1 = 0,
    ECOS_RCU_PLL_OD_2 = 1,
    ECOS_RCU_PLL_OD_4 = 2,
    ECOS_RCU_PLL_OD_8 = 3
} ecos_rcu_pll_od_t;

typedef enum {
    ECOS_RCU_CLOCK_PLL = 0,
    ECOS_RCU_CLOCK_CPU,
    ECOS_RCU_CLOCK_AXI,
    ECOS_RCU_CLOCK_AHB,
    ECOS_RCU_CLOCK_APB
} ecos_rcu_clock_domain_t;

/* keep_apb 为 true 时 APB 不切 PLL，保持原时钟，串口等 APB 外设不受影响。 */
typedef struct {
    uint8_t n;
    ecos_rcu_pll_select_t select;
    ecos_rcu_pll_od_t od;
    uint8_t cpu_div;
    uint8_t axi_div;
    uint8_t ahb_div;
    bool keep_apb;
    uint32_t settle_ticks;
} ecos_rcu_pll_config_t;

/* cl1-2512 已验证组合：Fvco=800MHz，Fpll=400MHz，CPU/AXI=200MHz，
 * AHB=100MHz，APB 保持 20MHz。 */
#define ECOS_RCU_PLL_CONFIG_DEFAULT \
    { 40u, ECOS_RCU_PLL_SELECT_X1, ECOS_RCU_PLL_OD_2, 2u, 2u, 4u, true, 0u }

ecos_err_t ecos_rcu_pll_apply(ecos_rcu_id_t id,
                              const ecos_rcu_pll_config_t *config);
ecos_err_t ecos_rcu_pll_get_freq(ecos_rcu_id_t id,
                                 ecos_rcu_clock_domain_t domain,
                                 uint32_t *freq_hz);

#ifdef __cplusplus
}
#endif

#endif
