#include "ecos/bsp/led.h"
#include "ecos/driver/timer.h"

#define BLINK_DELAY_MS 500u

int main(void)
{
    if (ecos_timer_get_instance_count() < 1)
        goto failed;
    if (bsp_led_init() != BSP_LED_OK)
        goto failed;

    for (;;) {
        if (bsp_led_set_state(BSP_LED_0, BSP_LED_ON) != BSP_LED_OK)
            break;
        if (ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, BLINK_DELAY_MS) !=
            ECOS_TIMER_OK)
            break;
        if (bsp_led_set_state(BSP_LED_0, BSP_LED_OFF) != BSP_LED_OK)
            break;
        if (ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, BLINK_DELAY_MS) !=
            ECOS_TIMER_OK)
            break;
    }

failed:
    for (;;)
        __asm__ volatile("nop");
}
