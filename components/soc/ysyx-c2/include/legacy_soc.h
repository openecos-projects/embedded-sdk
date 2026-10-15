#ifndef ECOS_LEGACY_SOC_C2_H
#define ECOS_LEGACY_SOC_C2_H

#include <stdint.h>
#include "legacy_soc_registers.h"

#define LEGACY_SOC_SIMPLE_GPIO 1
#define LEGACY_GPIO_COUNT 1
#define LEGACY_UART0_SIMPLE 1
#define LEGACY_HAS_UART1 1
#define LEGACY_HAS_GPIO_MUX 0
#define LEGACY_I2C_REGISTER_WIDTH 32
#define LEGACY_TIMER_STYLE_C 1
#define LEGACY_TIMER_COUNT 2
#define LEGACY_PWM_COUNT 1
#define LEGACY_HAS_ARCHINFO 1

#define LEGACY_UART1_INIT_PINS() do { } while (0)

#endif
