#ifndef ECOS_LEGACY_SOC_L3_1_H
#define ECOS_LEGACY_SOC_L3_1_H

#include <stdint.h>
#include "legacy_soc_registers.h"

#define LEGACY_SOC_PAD_GPIO 1
#define LEGACY_GPIO_COUNT 2
#define LEGACY_UART0_16550 1
#define LEGACY_HAS_UART1 1
#define LEGACY_HAS_GPIO_MUX 1
#define LEGACY_QSPI_GPIO1 1
#define LEGACY_I2C_REGISTER_WIDTH 8
#define LEGACY_TIMER_STYLE_NEW 1
#define LEGACY_TIMER_COUNT 1
#define LEGACY_PWM_COUNT 3
#define LEGACY_HAS_ARCHINFO 1
#define LEGACY_HAS_CRC 1
#define LEGACY_HAS_RNG 1
#define LEGACY_HAS_RTC 1
#define LEGACY_HAS_RCU 1

#define LEGACY_UART1_INIT_PINS() \
    do { \
        REG_GPIO_0_IOFCFG |= 0x3u; \
        REG_GPIO_0_PINMUX &= ~0x3u; \
    } while (0)

#endif
