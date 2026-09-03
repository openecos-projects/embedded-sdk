#ifndef ECOS_HAL_GPIO_H
#define ECOS_HAL_GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_GPIO_PORT_0 = 0,
    HAL_GPIO_PORT_1,
    HAL_GPIO_PORT_2,
    HAL_GPIO_PORT_COUNT
} hal_gpio_port_t;

typedef enum {
    HAL_GPIO_DIRECTION_INPUT = 0,
    HAL_GPIO_DIRECTION_OUTPUT
} hal_gpio_direction_t;

typedef enum {
    HAL_GPIO_LEVEL_LOW = 0,
    HAL_GPIO_LEVEL_HIGH
} hal_gpio_level_t;

typedef enum {
    HAL_GPIO_FUNCTION_GPIO = 0,
    HAL_GPIO_FUNCTION_ALT_0,
    HAL_GPIO_FUNCTION_ALT_1
} hal_gpio_function_t;

typedef struct {
    hal_gpio_direction_t direction;
    hal_gpio_function_t function;
} hal_gpio_config_t;

enum {
    HAL_GPIO_OK = 0,
    HAL_GPIO_ERROR_INVALID_ARGUMENT = -1,
    HAL_GPIO_ERROR_UNSUPPORTED = -2
};

int hal_gpio_configure(hal_gpio_port_t port,
                       uint8_t pin,
                       const hal_gpio_config_t *config);
int hal_gpio_set_direction(hal_gpio_port_t port,
                           uint8_t pin,
                           hal_gpio_direction_t direction);
int hal_gpio_set_level(hal_gpio_port_t port,
                       uint8_t pin,
                       hal_gpio_level_t level);
int hal_gpio_get_level(hal_gpio_port_t port, uint8_t pin);
int hal_gpio_set_function(hal_gpio_port_t port,
                          uint8_t pin,
                          hal_gpio_function_t function);

#ifdef __cplusplus
}
#endif

#endif
