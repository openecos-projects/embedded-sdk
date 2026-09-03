#include "ecos/bsp/console.h"
#include "ecos/bsp/led.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define BLINK_DELAY_MS 500u
#define LOG_TAG "blink"

int main(void)
{
    const char *operation = "initialize console";
    ecos_err_t result = bsp_console_init();

    if (ecos_result_failed(result))
        goto failed;

    (void)ECOS_LOGI(LOG_TAG, "Starting blink example");

    if (ecos_timer_get_instance_count() < 1) {
        operation = "find default timer";
        result = ECOS_ERR_NOT_FOUND;
        goto failed;
    }

    operation = "initialize LED";
    result = bsp_led_init();
    if (ecos_result_failed(result))
        goto failed;

    (void)ECOS_LOGI(LOG_TAG, "Blinking LED 0 every %u ms", BLINK_DELAY_MS);

    for (;;) {
        operation = "turn LED on";
        result = bsp_led_set_state(BSP_LED_0, BSP_LED_ON);
        if (ecos_result_failed(result))
            goto failed;

        operation = "wait after turning LED on";
        result = ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, BLINK_DELAY_MS);
        if (ecos_result_failed(result))
            goto failed;

        operation = "turn LED off";
        result = bsp_led_set_state(BSP_LED_0, BSP_LED_OFF);
        if (ecos_result_failed(result))
            goto failed;

        operation = "wait after turning LED off";
        result = ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, BLINK_DELAY_MS);
        if (ecos_result_failed(result))
            goto failed;
    }

failed:
    (void)operation;
    (void)ECOS_LOG_ERR(LOG_TAG, result, operation);
    for (;;)
        __asm__ volatile("nop");
}
