// System Headers
#include "string.h"
#include "stdio.h"
#include "libgcc.h"

// System Configuration
#include "generated/autoconf.h"

// Components headers
#include "hal_timer.h"
#include "hal_qspi.h"
#include "hal_gpio.h"
#include "hal_pwm.h"
#include "hal_sys_uart.h"
#include "hal_i2c.h"

// Device headers
#include "sgp30.h"

#define PSRAM_SCKL_FREQ_MHZ (CONFIG_CPU_FREQ_MHZ / CONFIG_PSRAM_NUM)
