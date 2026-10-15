#include "ecos/board_resources.h"
#include "ecos/bsp/console.h"
#include "ecos/driver/gpio.h"
#include "ecos/driver/gpio_interrupt.h"
#include "ecos/driver/interrupt.h"
#include "ecos/log.h"

#include <stddef.h>
#include <stdint.h>

#define LOG_TAG "gpio-interrupt"

#if !ECOS_BOARD_HAS_GPIO_DEMO
#error "gpio-interrupt requires the selected Board to provide gpio-demo"
#endif

static volatile uint32_t edge_count;

static void gpio_handler(void *arg)
{
    (void)arg;
    ++edge_count;
}

int main(void)
{
    const ecos_gpio_pin_t input_pin = ECOS_BOARD_GPIO_DEMO_INPUT;
    const ecos_gpio_config_t config = {
        ECOS_GPIO_DIRECTION_INPUT,
        ECOS_GPIO_FUNCTION_GPIO
    };

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_configure(input_pin.port, input_pin.pin, &config),
        "configure interrupt input"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_interrupt_set_trigger(
            input_pin.port, input_pin.pin, ECOS_GPIO_INTERRUPT_ANYEDGE
        ),
        "configure input interrupt"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_interrupt_handler_add(
            input_pin.port, input_pin.pin, gpio_handler, NULL, 1u
        ),
        "register input interrupt"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_interrupt_enable(input_pin.port, input_pin.pin),
        "enable input interrupt"
    );
    ecos_interrupt_set_threshold(0u);
    ecos_interrupt_global_enable();
    (void)ECOS_LOGI(LOG_TAG, "%s interrupt ready",
                    ECOS_BOARD_GPIO_DEMO_INPUT_LABEL);
    for (uint32_t reported = 0u;;) {
        uint32_t current = edge_count;

        if (current != reported) {
            (void)ECOS_LOGI(LOG_TAG, "edges: %u", current);
            reported = current;
        }
    }
}
