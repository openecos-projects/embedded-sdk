#include "ecos/bsp/console.h"
#include "ecos/driver/interrupt.h"
#include "ecos/driver/timer.h"
#include "ecos/driver/timer_interrupt.h"
#include "ecos/log.h"

#include <stddef.h>
#include <stdint.h>

#define LOG_TAG "timer-interrupt"

static volatile uint32_t tick_count;

static void timer_handler(void *arg)
{
    (void)arg;
    ++tick_count;
}

int main(void)
{
    const ecos_timer_config_t config = { 1000000u };

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_timer_init(ECOS_TIMER_DEFAULT, &config),
        "initialize timer"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_timer_register_callback(
            ECOS_TIMER_DEFAULT, timer_handler, NULL, 1u
        ),
        "register timer callback"
    );
    ecos_interrupt_set_threshold(0u);
    ecos_interrupt_global_enable();
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_timer_start(ECOS_TIMER_DEFAULT),
        "start timer"
    );
    (void)ECOS_LOGI(LOG_TAG, "Timer0 interrupt ready");
    for (uint32_t reported = 0u;;) {
        uint32_t current = tick_count;

        if (current != reported) {
            (void)ECOS_LOGI(LOG_TAG, "ticks: %u", current);
            reported = current;
        }
    }
}
