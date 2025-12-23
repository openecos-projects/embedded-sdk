// System Headers
#include "string.h"
#include "stdio.h"
#include "libgcc.h"

// System Configuration
#include "generated/autoconf.h"

// Components headers
// #include "timer.h"
// #include "qspi.h"
#include "gpio.h"
// #include "pwm.h"
// #include "hp_uart.h"
#include "i2c.h"

// Device headers
// #include "st7735.h"
// #include "sgp30.h"

#define PSRAM_SCKL_FREQ_MHZ (CONFIG_CPU_FREQ_MHZ / CONFIG_PSRAM_NUM)