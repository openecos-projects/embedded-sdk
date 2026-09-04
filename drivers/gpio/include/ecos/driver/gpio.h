#ifndef ECOS_DRIVER_GPIO_H
#define ECOS_DRIVER_GPIO_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_GPIO_PORT_0 = 0,
    ECOS_GPIO_PORT_1,
    ECOS_GPIO_PORT_2,
    ECOS_GPIO_PORT_3,
    ECOS_GPIO_PORT_COUNT
} ecos_gpio_port_t;

typedef enum {
    ECOS_GPIO_DIRECTION_INPUT = 0,
    ECOS_GPIO_DIRECTION_OUTPUT
} ecos_gpio_direction_t;

typedef enum {
    ECOS_GPIO_LEVEL_LOW = 0,
    ECOS_GPIO_LEVEL_HIGH
} ecos_gpio_level_t;

typedef enum {
    ECOS_GPIO_FUNCTION_GPIO = 0,
    ECOS_GPIO_FUNCTION_ALT_0,
    ECOS_GPIO_FUNCTION_ALT_1
} ecos_gpio_function_t;

typedef struct {
    ecos_gpio_direction_t direction;
    ecos_gpio_function_t function;
} ecos_gpio_config_t;

#define ECOS_GPIO_CONFIG_DEFAULT \
    { ECOS_GPIO_DIRECTION_INPUT, ECOS_GPIO_FUNCTION_GPIO }

/* Configure one pin. Valid pin ranges are defined by the selected Target. */
ecos_err_t ecos_gpio_configure(ecos_gpio_port_t port,
                               uint8_t pin,
                               const ecos_gpio_config_t *config);

ecos_err_t ecos_gpio_set_direction(ecos_gpio_port_t port,
                                   uint8_t pin,
                                   ecos_gpio_direction_t direction);
ecos_err_t ecos_gpio_set_level(ecos_gpio_port_t port,
                               uint8_t pin,
                               ecos_gpio_level_t level);

/* Returns ECOS_GPIO_LEVEL_LOW/HIGH or a negative error code. */
int ecos_gpio_get_level(ecos_gpio_port_t port, uint8_t pin);

ecos_err_t ecos_gpio_set_function(ecos_gpio_port_t port,
                                  uint8_t pin,
                                  ecos_gpio_function_t function);

#ifdef __cplusplus
}
#endif

#endif
