#include "ecos/bsp/console.h"
#include "ecos/driver/rcu.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "rcu-pll"
#define HEARTBEAT_INTERVAL_S 5u

/* 默认配置为已验证组合，置于 rodata 以避免运行期结构体拷贝。 */
static const ecos_rcu_pll_config_t pll_config = ECOS_RCU_PLL_CONFIG_DEFAULT;

static void log_clock_domain(ecos_rcu_clock_domain_t domain, const char *name)
{
    uint32_t freq_hz;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_rcu_pll_get_freq(ECOS_RCU_DEFAULT, domain, &freq_hz),
        "read clock frequency"
    );
    (void)ECOS_LOGI(LOG_TAG, "%s: %u Hz", name, freq_hz);
}

static void log_all_clocks(const char *stage)
{
    uint32_t status;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_rcu_get_status(ECOS_RCU_DEFAULT, &status),
        "read RCU status"
    );
    (void)ECOS_LOGI(LOG_TAG, "%s: CLK_CTRL=0x%08X", stage, status);
    log_clock_domain(ECOS_RCU_CLOCK_PLL, "PLL");
    log_clock_domain(ECOS_RCU_CLOCK_CPU, "CPU");
    log_clock_domain(ECOS_RCU_CLOCK_AXI, "AXI");
    log_clock_domain(ECOS_RCU_CLOCK_AHB, "AHB");
    log_clock_domain(ECOS_RCU_CLOCK_APB, "APB");
}

int main(void)
{
    uint32_t heartbeat = 0u;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    (void)ECOS_LOGI(
        LOG_TAG, "[TEST_START] switch CPU/AXI/AHB to PLL, keep APB unchanged"
    );

    log_all_clocks("before");

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_rcu_pll_apply(ECOS_RCU_DEFAULT, &pll_config),
        "apply PLL config"
    );

    log_all_clocks("after");
    (void)ECOS_LOGI(
        LOG_TAG,
        "[TEST_PASS] PLL switched, heartbeat every %u s",
        (unsigned)HEARTBEAT_INTERVAL_S
    );

    for (;;) {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_s(ECOS_TIMER_DEFAULT, HEARTBEAT_INTERVAL_S),
            "wait between heartbeats"
        );
        (void)ECOS_LOGI(LOG_TAG, "heartbeat %u", heartbeat++);
    }
}
