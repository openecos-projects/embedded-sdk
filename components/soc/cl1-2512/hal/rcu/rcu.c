#include "ecos/hal/rcu.h"
#include "cl1_2512_soc.h"

#include <stddef.h>
#include <stdint.h>

/* PLL 稳定等待默认值（mtime ticks，5 MHz 时约 40 us）。 */
#define CL1_2512_PLL_SETTLE_TICKS_DEFAULT 200u

/* CLK_DIV 的 APB 字节保持参考值 0x08。 */
#define CL1_2512_CRU_CLK_DIV_APB_KEEP     0x08u

#define CL1_2512_PLL_FVCO_MIN_HZ          500000000UL
#define CL1_2512_PLL_FVCO_MAX_HZ          1200000000UL

static int rcu_id_is_valid(hal_rcu_id_t id)
{
    return id == HAL_RCU_ID_0;
}

/**
 * 稳定读取核心本地计时器的 64 位当前值。
 */
static uint64_t cru_read_mtime(void)
{
    uint32_t high_before;
    uint32_t low;
    uint32_t high_after;

    do {
        high_before = REG_CLINT_MTIME_HIGH;
        low = REG_CLINT_MTIME_LOW;
        high_after = REG_CLINT_MTIME_HIGH;
    } while (high_before != high_after);

    return ((uint64_t)high_after << 32) | low;
}

/**
 * 使用核心本地计时器等待指定数量的计时周期。
 */
static void cru_delay_ticks(uint64_t ticks)
{
    uint64_t start = cru_read_mtime();

    while ((cru_read_mtime() - start) < ticks) {
    }
}

ecos_err_t hal_rcu_init(hal_rcu_id_t id, const hal_rcu_config_t *config)
{
    /* cl1-2512 无 RDIV/CTRL 寄存器，仅 PLL 接口可用。 */
    (void)id;
    (void)config;
    return ECOS_ERR_UNSUPPORTED;
}

ecos_err_t hal_rcu_deinit(hal_rcu_id_t id)
{
    (void)id;
    return ECOS_ERR_UNSUPPORTED;
}

ecos_err_t hal_rcu_get_status(hal_rcu_id_t id, uint32_t *status)
{
    if (!rcu_id_is_valid(id) || status == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    *status = REG_CRU_CLK_CTRL;
    return ECOS_OK;
}

ecos_err_t hal_rcu_pll_apply(hal_rcu_id_t id, const hal_rcu_pll_config_t *config)
{
    uint32_t fvco_hz;
    uint32_t settle_ticks;

    if (!rcu_id_is_valid(id) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->n == 0u ||
        (unsigned)config->select > (unsigned)HAL_RCU_PLL_SELECT_X2 ||
        (unsigned)config->od > (unsigned)HAL_RCU_PLL_OD_8)
        return ECOS_ERR_INVALID_ARGUMENT;
    /* SELECT 倍频无文档化寄存器位，当前仅支持 X1。 */
    if (config->select != HAL_RCU_PLL_SELECT_X1)
        return ECOS_ERR_UNSUPPORTED;
    /* n 过大时 Fvco 乘积溢出 uint32，且必然超出 PLL 合法区间。 */
    if ((uint32_t)config->n > UINT32_MAX / CL1_2512_PLL_REF_HZ / 2u)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* Fvco = Fref x N x (SELECT ? 2 : 1)，必须落在 PLL 合法区间。 */
    fvco_hz = CL1_2512_PLL_REF_HZ * config->n *
              (config->select == HAL_RCU_PLL_SELECT_X2 ? 2u : 1u);
    if (fvco_hz < CL1_2512_PLL_FVCO_MIN_HZ ||
        fvco_hz > CL1_2512_PLL_FVCO_MAX_HZ)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* 配置 CPU、AXI、AHB 和 APB 的时钟分频参数。 */
    REG_CRU_CLK_DIV =
        ((uint32_t)CL1_2512_CRU_CLK_DIV_APB_KEEP <<
         CL1_2512_CRU_CLK_DIV_APB_SHIFT) |
        ((uint32_t)config->ahb_div << CL1_2512_CRU_CLK_DIV_AHB_SHIFT) |
        ((uint32_t)config->axi_div << CL1_2512_CRU_CLK_DIV_AXI_SHIFT) |
        ((uint32_t)config->cpu_div << CL1_2512_CRU_CLK_DIV_CPU_SHIFT);

    /* 写入 PLL 参数并通过控制寄存器完成复位释放。 */
    REG_CRU_PLL_CTRL2 = (uint32_t)config->n << CL1_2512_PLL_CTRL2_N_SHIFT;
    REG_CRU_PLL_CTRL0 = CL1_2512_PLL_CTRL0_ENABLE |
                        ((uint32_t)config->od << CL1_2512_PLL_CTRL0_OD_SHIFT);
    REG_CRU_CLK_CTRL = CL1_2512_CRU_CLK_CTRL_SWITCH_ALL;
    REG_CRU_PLL_CTRL3 = 0u;
    REG_CRU_PLL_CTRL3 = 1u;

    /* 等待 PLL 参数稳定后切换时钟，APB 按需保持原时钟。 */
    settle_ticks = config->settle_ticks != 0u
                       ? config->settle_ticks
                       : CL1_2512_PLL_SETTLE_TICKS_DEFAULT;
    cru_delay_ticks(settle_ticks);
    REG_CRU_CLK_CTRL = config->keep_apb ? CL1_2512_CRU_CLK_CTRL_KEEP_APB
                                        : CL1_2512_CRU_CLK_CTRL_SWITCH_ALL;

    /* 等待时钟寄存器写入及后续取指操作全部生效。 */
    __asm__ volatile("fence rw, rw\n\tfence.i" ::: "memory");
    return ECOS_OK;
}

ecos_err_t hal_rcu_pll_get_freq(hal_rcu_id_t id, hal_rcu_clock_domain_t domain,
                                uint32_t *freq_hz)
{
    uint32_t freq;
    uint32_t clk_div;
    uint32_t divider;
    uint32_t n;
    uint32_t od;

    if (!rcu_id_is_valid(id) || freq_hz == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if ((unsigned)domain > (unsigned)HAL_RCU_CLOCK_APB)
        return ECOS_ERR_INVALID_ARGUMENT;

    n = (REG_CRU_PLL_CTRL2 & CL1_2512_PLL_CTRL2_N_MASK) >>
        CL1_2512_PLL_CTRL2_N_SHIFT;
    if (n == 0u) {
        /* PLL 未使能，各时钟域运行于参考时钟。 */
        *freq_hz = domain == HAL_RCU_CLOCK_PLL ? 0u : CL1_2512_PLL_REF_HZ;
        return ECOS_OK;
    }
    /* 无合法 PLL 配置会产生这样的寄存器状态，且乘积会溢出 uint32。 */
    if (n > UINT32_MAX / CL1_2512_PLL_REF_HZ)
        return ECOS_ERR_INVALID_STATE;

    od = (REG_CRU_PLL_CTRL0 & CL1_2512_PLL_CTRL0_OD_MASK) >>
         CL1_2512_PLL_CTRL0_OD_SHIFT;
    freq = (CL1_2512_PLL_REF_HZ * n) >> od;

    if (domain == HAL_RCU_CLOCK_APB &&
        (REG_CRU_CLK_CTRL & CL1_2512_CRU_CLK_CTRL_KEEP_APB) != 0u) {
        *freq_hz = CL1_2512_PLL_REF_HZ;
        return ECOS_OK;
    }

    clk_div = REG_CRU_CLK_DIV;
    switch (domain) {
    case HAL_RCU_CLOCK_PLL:
        divider = 1u;
        break;
    case HAL_RCU_CLOCK_CPU:
        divider = (clk_div >> CL1_2512_CRU_CLK_DIV_CPU_SHIFT) &
                  CL1_2512_CRU_CLK_DIV_FIELD_MASK;
        break;
    case HAL_RCU_CLOCK_AXI:
        divider = (clk_div >> CL1_2512_CRU_CLK_DIV_AXI_SHIFT) &
                  CL1_2512_CRU_CLK_DIV_FIELD_MASK;
        break;
    case HAL_RCU_CLOCK_AHB:
        divider = (clk_div >> CL1_2512_CRU_CLK_DIV_AHB_SHIFT) &
                  CL1_2512_CRU_CLK_DIV_FIELD_MASK;
        break;
    default:
        divider = (clk_div >> CL1_2512_CRU_CLK_DIV_APB_SHIFT) &
                  CL1_2512_CRU_CLK_DIV_FIELD_MASK;
        break;
    }
    if (divider == 0u)
        return ECOS_ERR_INVALID_STATE;

    *freq_hz = freq / divider;
    return ECOS_OK;
}
