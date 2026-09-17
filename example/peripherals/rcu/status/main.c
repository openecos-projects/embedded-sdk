#include "ecos/bsp/console.h"
#include "ecos/driver/rcu.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "rcu-status"

/* 控制位含义由 SoC 定义；0xB 是 2.x 冒烟使用的值，可按硬件文档调整。 */
#define RCU_CLOCK_DIVIDER 256u
#define RCU_CONTROL_BITS   0xBu

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    const ecos_rcu_config_t config = { RCU_CLOCK_DIVIDER, RCU_CONTROL_BITS };
    uint32_t status;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_rcu_init(ECOS_RCU_DEFAULT, &config), "initialize RCU"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_rcu_get_status(ECOS_RCU_DEFAULT, &status),
        "read RCU status"
    );
    (void)ECOS_LOGI(
        LOG_TAG,
        "RCU status: 0x%08X (divider %u, control 0x%X)",
        status,
        (unsigned)RCU_CLOCK_DIVIDER,
        (unsigned)RCU_CONTROL_BITS
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_rcu_deinit(ECOS_RCU_DEFAULT), "deinitialize RCU"
    );
    halt();
}
