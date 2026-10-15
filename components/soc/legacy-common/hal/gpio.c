#include "ecos/hal/gpio.h"
#include "legacy_soc.h"

#include <stddef.h>

typedef struct {
    volatile uint32_t *direction;
    volatile uint32_t *input;
    volatile uint32_t *output;
    volatile uint32_t *function;
    volatile uint32_t *mux;
} gpio_registers_t;

#if LEGACY_SOC_SIMPLE_GPIO
static uint32_t simple_direction;
#if defined(CONFIG_LINK_RAM_REGION_PSRAM)
static uint32_t simple_output = 0x8000u;
#else
static uint32_t simple_output;
#endif
#endif

static int port_is_valid(hal_gpio_port_t port)
{
    return port >= HAL_GPIO_PORT_0 && port < LEGACY_GPIO_COUNT;
}

static int pin_is_valid(uint8_t pin)
{
    return pin < 32u;
}

static int get_registers(hal_gpio_port_t port, gpio_registers_t *registers)
{
    if (registers == NULL || !port_is_valid(port))
        return ECOS_ERR_INVALID_ARGUMENT;
#if LEGACY_SOC_SIMPLE_GPIO
    if (port != HAL_GPIO_PORT_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    registers->direction = &REG_GPIO_0_DDR;
    registers->input = &REG_GPIO_0_DR;
    registers->output = &REG_GPIO_0_DR;
    registers->function = NULL;
    registers->mux = NULL;
#else
    switch (port) {
    case HAL_GPIO_PORT_0:
        registers->direction = &REG_GPIO_0_PADDIR;
        registers->input = &REG_GPIO_0_PADIN;
        registers->output = &REG_GPIO_0_PADOUT;
        registers->function = &REG_GPIO_0_IOFCFG;
        registers->mux = &REG_GPIO_0_PINMUX;
        break;
    case HAL_GPIO_PORT_1:
        registers->direction = &REG_GPIO_1_PADDIR;
        registers->input = &REG_GPIO_1_PADIN;
        registers->output = &REG_GPIO_1_PADOUT;
        registers->function = &REG_GPIO_1_IOFCFG;
        registers->mux = &REG_GPIO_1_PINMUX;
        break;
    default:
        return ECOS_ERR_INVALID_ARGUMENT;
    }
#endif
    return ECOS_OK;
}

ecos_err_t hal_gpio_set_direction(hal_gpio_port_t port, uint8_t pin,
                                  hal_gpio_direction_t direction)
{
    gpio_registers_t registers;
    uint32_t bit;
    int result;
    if (!pin_is_valid(pin) || (direction != HAL_GPIO_DIRECTION_INPUT &&
                               direction != HAL_GPIO_DIRECTION_OUTPUT))
        return ECOS_ERR_INVALID_ARGUMENT;
    result = get_registers(port, &registers);
    if (result != ECOS_OK)
        return result;
    bit = (uint32_t)1u << pin;
#if LEGACY_SOC_SIMPLE_GPIO
    if (direction == HAL_GPIO_DIRECTION_INPUT)
        simple_direction |= bit;
    else
        simple_direction &= ~bit;
    *registers.direction = simple_direction;
#else
    if (direction == HAL_GPIO_DIRECTION_OUTPUT)
        *registers.direction |= bit;
    else
        *registers.direction &= ~bit;
#endif
    return ECOS_OK;
}

ecos_err_t hal_gpio_set_level(hal_gpio_port_t port, uint8_t pin,
                              hal_gpio_level_t level)
{
    gpio_registers_t registers;
    uint32_t bit;
    int result;
    if (!pin_is_valid(pin) || (level != HAL_GPIO_LEVEL_LOW &&
                               level != HAL_GPIO_LEVEL_HIGH))
        return ECOS_ERR_INVALID_ARGUMENT;
    result = get_registers(port, &registers);
    if (result != ECOS_OK)
        return result;
    bit = (uint32_t)1u << pin;
#if LEGACY_SOC_SIMPLE_GPIO
    if (level == HAL_GPIO_LEVEL_HIGH)
        simple_output |= bit;
    else
        simple_output &= ~bit;
    *registers.output = simple_output;
#else
    if (level == HAL_GPIO_LEVEL_HIGH)
        *registers.output |= bit;
    else
        *registers.output &= ~bit;
#endif
    return ECOS_OK;
}

int hal_gpio_get_level(hal_gpio_port_t port, uint8_t pin)
{
    gpio_registers_t registers;
    uint32_t bit;
    int result;
    if (!pin_is_valid(pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    result = get_registers(port, &registers);
    if (result != ECOS_OK)
        return result;
    bit = (uint32_t)1u << pin;
#if LEGACY_SOC_SIMPLE_GPIO
    if ((simple_direction & bit) == 0u)
        return (simple_output & bit) != 0u ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
#endif
    return (*registers.input & bit) != 0u ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
}

ecos_err_t hal_gpio_set_function(hal_gpio_port_t port, uint8_t pin,
                                 hal_gpio_function_t function)
{
    gpio_registers_t registers;
#if !LEGACY_SOC_SIMPLE_GPIO
    uint32_t bit;
#endif
    int result;
    if (!pin_is_valid(pin) || function < HAL_GPIO_FUNCTION_GPIO ||
        function > HAL_GPIO_FUNCTION_ALT_1)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = get_registers(port, &registers);
    if (result != ECOS_OK)
        return result;
#if LEGACY_SOC_SIMPLE_GPIO
    return function == HAL_GPIO_FUNCTION_GPIO ? ECOS_OK : ECOS_ERR_UNSUPPORTED;
#else
    bit = (uint32_t)1u << pin;
    if (function == HAL_GPIO_FUNCTION_GPIO) {
        *registers.function &= ~bit;
    } else {
        if (function == HAL_GPIO_FUNCTION_ALT_1)
            *registers.mux |= bit;
        else
            *registers.mux &= ~bit;
        *registers.function |= bit;
    }
    return ECOS_OK;
#endif
}

ecos_err_t hal_gpio_configure(hal_gpio_port_t port, uint8_t pin,
                              const hal_gpio_config_t *config)
{
    int result;
    if (config == NULL || !pin_is_valid(pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    result = hal_gpio_set_direction(port, pin, config->direction);
    if (result != ECOS_OK)
        return result;
    return hal_gpio_set_function(port, pin, config->function);
}
