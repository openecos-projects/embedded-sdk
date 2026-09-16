#include "ecos/bsp/console.h"
#include "ecos/device/tm1650.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "gpio-tm1650"
#define COUNT_STEP_MS 100u
#define COUNT_MAX 9999u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    const ecos_tm1650_config_t display_config = ECOS_TM1650_CONFIG_DEFAULT;
    ecos_tm1650_t display;
    uint16_t counter = 0u;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_tm1650_init(&display, &display_config),
        "initialize TM1650"
    );

    (void)ECOS_LOGI(
        LOG_TAG,
        "TM1650 ready (DAT=GPIO%u[%u], CLK=GPIO%u[%u]), counting",
        (unsigned)display_config.dat_port,
        (unsigned)display_config.dat_pin,
        (unsigned)display_config.clk_port,
        (unsigned)display_config.clk_pin
    );

    for (;;) {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_tm1650_show_number(&display, counter, false),
            "show counter"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, COUNT_STEP_MS),
            "wait for next count"
        );
        counter = counter >= COUNT_MAX ? 0u : (uint16_t)(counter + 1u);
    }

    halt();
}
