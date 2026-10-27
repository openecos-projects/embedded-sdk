#include "ecos/bsp/console.h"
#include "ecos/driver/interrupt.h"
#include "ecos/driver/systick.h"
#include "ecos/log.h"

#define LOG_TAG "systick-heartbeat"
static volatile uint32_t ticks;
static void tick(void *arg) { (void)arg; ++ticks; }

int main(void)
{
    const ecos_systick_config_t config = { 1000000u };
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_interrupt_init(), "initialize interrupt");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_systick_init(&config, tick, NULL, 1u),
                        "initialize SysTick");
    ecos_interrupt_global_enable();
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_systick_start(), "start SysTick");
    for (;;) {
        if (ticks != 0u) {
            uint32_t value = ticks;
            ticks = 0u;
            (void)ECOS_LOGI(LOG_TAG, "tick %u", (unsigned)value);
        }
    }
}
