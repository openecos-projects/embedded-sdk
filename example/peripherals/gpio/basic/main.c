#include "ecos/board_resources.h"
#include "ecos/bsp/console.h"
#include "ecos/driver/gpio.h"
#include "ecos/log.h"

#if !ECOS_BOARD_HAS_GPIO_DEMO
#error "gpio-basic requires the selected Board to provide gpio-demo"
#endif

#define LOG_TAG "gpio-basic"

int main(void)
{
    const ecos_gpio_pin_t output_pin = ECOS_BOARD_GPIO_DEMO_OUTPUT;
    const ecos_gpio_pin_t input_pin = ECOS_BOARD_GPIO_DEMO_INPUT;
    const ecos_gpio_config_t output_config = {
        .direction = ECOS_GPIO_DIRECTION_OUTPUT,
        .function = ECOS_GPIO_FUNCTION_GPIO,
    };
    const ecos_gpio_config_t input_config = {
        .direction = ECOS_GPIO_DIRECTION_INPUT,
        .function = ECOS_GPIO_FUNCTION_GPIO,
    };
    int previous_level = -1;
    int input_level;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_set_level(
            output_pin.port,
            output_pin.pin,
            ECOS_BOARD_GPIO_DEMO_OUTPUT_INITIAL_LEVEL
        ),
        "set initial output level"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_configure(output_pin.port, output_pin.pin, &output_config),
        "configure output pin"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_gpio_configure(input_pin.port, input_pin.pin, &input_config),
        "configure input pin"
    );

    (void)ECOS_LOGI(
        LOG_TAG,
        "Mirroring %s input to %s output",
        ECOS_BOARD_GPIO_DEMO_INPUT_LABEL,
        ECOS_BOARD_GPIO_DEMO_OUTPUT_LABEL
    );

    for (;;) {
        input_level = ecos_gpio_get_level(input_pin.port, input_pin.pin);
        ECOS_PANIC_ON_ERROR(LOG_TAG, input_level, "read input pin");

        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_gpio_set_level(
                output_pin.port,
                output_pin.pin,
                (ecos_gpio_level_t)input_level
            ),
            "write output pin"
        );

        if (input_level != previous_level) {
            (void)ECOS_LOGI(
                LOG_TAG,
                "Input=%s, output=%s",
                input_level == ECOS_GPIO_LEVEL_HIGH ? "HIGH" : "LOW",
                input_level == ECOS_GPIO_LEVEL_HIGH ? "HIGH" : "LOW"
            );
            previous_level = input_level;
        }
    }
}
