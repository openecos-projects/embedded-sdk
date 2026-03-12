// System Headers
#include "string.h"
#include "stdio.h"
#include "libgcc.h"

// System Configuration
#include "generated/autoconf.h"

// HAL headers (replacing component headers)
#include "hal/sys_uart.h"
// #include "hal/timer.h"
// #include "hal/qspi.h"
// #include "hal/gpio.h"
// #include "hal/pwm.h"
// #include "hal/hp_uart.h"
// #include "hal/i2c.h"

// Device headers
// #include "st7735.h"
// #include "sgp30.h"

#define PSRAM_SCKL_FREQ_MHZ (CONFIG_CPU_FREQ_MHZ / CONFIG_PSRAM_NUM)