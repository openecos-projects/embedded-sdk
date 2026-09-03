#include "ecos/bsp/led.h"

#include <stdint.h>

#define BLINK_DELAY_ITERATIONS 5000000u

static void blink_delay(void)
{
    volatile uint32_t count;

    for (count = 0u; count < BLINK_DELAY_ITERATIONS; ++count)
        __asm__ volatile("nop");
}

int main(void)
{
    if (bsp_led_init() != BSP_LED_OK)
        goto failed;

    for (;;) {
        if (bsp_led_set_state(BSP_LED_0, BSP_LED_ON) != BSP_LED_OK)
            break;
        blink_delay();
        if (bsp_led_set_state(BSP_LED_0, BSP_LED_OFF) != BSP_LED_OK)
            break;
        blink_delay();
    }

failed:
    for (;;)
        __asm__ volatile("nop");
}
