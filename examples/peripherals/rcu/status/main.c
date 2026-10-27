#include "ecos/bsp/console.h"
#include "ecos/driver/rcu.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "rcu-status"

/* Control bits and divider are SoC-specific.  CH32 stays on HSI/8 MHz so
 * the USART1 console remains valid after the status read. */
#if defined(ECOS_SOC_CH32V103)
#define RCU_CLOCK_DIVIDER 1u
#define RCU_CONTROL_BITS  0u
#else
#define RCU_CLOCK_DIVIDER 256u
#define RCU_CONTROL_BITS  0xBu
#endif

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
