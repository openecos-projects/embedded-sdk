#include "ecos/board_resources.h"
#include "ecos/bsp/console.h"
#include "ecos/driver/gpio.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "timer-polling"

#if !ECOS_BOARD_HAS_GPIO_DEMO
#error "timer-polling requires the selected Board to provide gpio-demo"
#endif

int main(void)
{
    const ecos_gpio_pin_t output_pin = ECOS_BOARD_GPIO_DEMO_OUTPUT;
    const ecos_gpio_config_t output_config = {
        .direction = ECOS_GPIO_DIRECTION_OUTPUT,
        .function = ECOS_GPIO_FUNCTION_GPIO,
    };
    const ecos_timer_config_t config = { 10000u };
    ecos_gpio_level_t level = ECOS_GPIO_LEVEL_LOW;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_configure(output_pin.port, output_pin.pin, &output_config),
        "configure timer test output"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_set_level(output_pin.port, output_pin.pin, level),
        "set initial timer test output"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_timer_init(ECOS_TIMER_DEFAULT, &config),
        "initialize timer"
    );
    (void)ECOS_LOGI(
        LOG_TAG,
        "TIM2 polling ready: 10 ms period, HSI 8 MHz"
    );

    for (uint32_t sample = 0u; sample < 5u; ++sample) {
        int expired;

        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_start(ECOS_TIMER_DEFAULT),
            "start timer"
        );
        do {
            expired = ecos_timer_is_expired(ECOS_TIMER_DEFAULT);
        } while (expired == 0);
        ECOS_PANIC_ON_ERROR(LOG_TAG, expired, "poll timer status");
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_stop(ECOS_TIMER_DEFAULT),
            "stop timer"
        );
        level = level == ECOS_GPIO_LEVEL_LOW ?
                ECOS_GPIO_LEVEL_HIGH : ECOS_GPIO_LEVEL_LOW;
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_gpio_set_level(output_pin.port, output_pin.pin, level),
            "toggle timer test output"
        );
        (void)ECOS_LOGI(
            LOG_TAG,
            "period elapsed: %u, output=%s",
            (unsigned)(sample + 1u),
            level == ECOS_GPIO_LEVEL_HIGH ? "HIGH" : "LOW"
        );
    }

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_timer_deinit(ECOS_TIMER_DEFAULT),
        "deinitialize timer"
    );
    (void)ECOS_LOGI(LOG_TAG, "timer polling completed");
    for (;;)
        __asm__ volatile("nop");
}
