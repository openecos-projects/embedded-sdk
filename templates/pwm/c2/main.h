// System Headers
#include "string.h"
#include "stdio.h"
#include "libgcc.h"

// System Configuration
#include "generated/autoconf.h"

// HAL headers (uncomment as needed)
#include "hal/common.h"
#include "hal/sys_uart.h"
#include "hal/pwm.h"
// #include "hal/gpio.h"
// #include "hal/hp_uart.h"
// #include "hal/timer.h"
// #include "hal/i2c.h"
// #include "hal/qspi.h"

#define PSRAM_SCKL_FREQ_MHZ (CONFIG_CPU_FREQ_MHZ / CONFIG_PSRAM_NUM)