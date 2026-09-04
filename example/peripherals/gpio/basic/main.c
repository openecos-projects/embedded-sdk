#include "ecos/bsp/console.h"
#include "ecos/driver/gpio.h"
#include "ecos/log.h"

#define GPIO_DEMO_PORT ECOS_GPIO_PORT_1
#define GPIO_OUTPUT_PIN 5u
#define GPIO_INPUT_PIN 7u
#define LOG_TAG "gpio-basic"

static void stop_with_error(ecos_err_t error, const char *operation)
{
    (void)ECOS_LOG_ERR(LOG_TAG, error, operation);
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
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
    ecos_err_t result;

    result = bsp_console_init();
    if (ecos_result_failed(result))
        stop_with_error(result, "initialize console");

    /* GPIO1[5] is LED 0 and is active-low on StarrySky L4. */
    result = ecos_gpio_set_level(
        GPIO_DEMO_PORT, GPIO_OUTPUT_PIN, ECOS_GPIO_LEVEL_HIGH
    );
    if (ecos_result_failed(result))
        stop_with_error(result, "set initial output level");

    result = ecos_gpio_configure(
        GPIO_DEMO_PORT, GPIO_OUTPUT_PIN, &output_config
    );
    if (ecos_result_failed(result))
        stop_with_error(result, "configure output pin");

    /* GPIO1[7] is button 0 and is also active-low on StarrySky L4. */
    result = ecos_gpio_configure(
        GPIO_DEMO_PORT, GPIO_INPUT_PIN, &input_config
    );
    if (ecos_result_failed(result))
        stop_with_error(result, "configure input pin");

    (void)ECOS_LOGI(
        LOG_TAG,
        "Mirroring GPIO1[%u] input to GPIO1[%u] output",
        GPIO_INPUT_PIN,
        GPIO_OUTPUT_PIN
    );

    for (;;) {
        input_level = ecos_gpio_get_level(GPIO_DEMO_PORT, GPIO_INPUT_PIN);
        if (input_level < 0)
            stop_with_error((ecos_err_t)input_level, "read input pin");

        result = ecos_gpio_set_level(
            GPIO_DEMO_PORT,
            GPIO_OUTPUT_PIN,
            (ecos_gpio_level_t)input_level
        );
        if (ecos_result_failed(result))
            stop_with_error(result, "write output pin");

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
