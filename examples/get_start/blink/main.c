#include "ecos/bsp/console.h"
#include "ecos/bsp/led.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define BLINK_DELAY_MS 500u
#define LOG_TAG "blink"

int main(void)
{
    int timer_count;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );

    (void)ECOS_LOGI(LOG_TAG, "Starting blink example");

    timer_count = ecos_timer_get_instance_count();
    ECOS_PANIC_ON_ERROR(LOG_TAG, timer_count, "query timers");
    if (timer_count < 1)
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ECOS_ERR_NOT_FOUND, "find default timer"
        );

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_led_init(), "initialize LED");

    (void)ECOS_LOGI(LOG_TAG, "Blinking LED 0 every %u ms", BLINK_DELAY_MS);

    for (;;) {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            bsp_led_set_state(BSP_LED_0, BSP_LED_ON),
            "turn LED on"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, BLINK_DELAY_MS),
            "wait after turning LED on"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            bsp_led_set_state(BSP_LED_0, BSP_LED_OFF),
            "turn LED off"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, BLINK_DELAY_MS),
            "wait after turning LED off"
        );
    }
}
