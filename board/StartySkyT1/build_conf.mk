# 根据配置加入 StartySky T1 系统串口驱动。
ifdef CONFIG_DRIVER_SYS_UART
SRC_PATH += $(BOARD_DRIVER_DIR)/sys_uart/sys_uart.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/sys_uart
endif

# 根据配置加入 StartySky T1 GPIO 驱动。
ifdef CONFIG_DRIVER_GPIO
SRC_PATH += $(BOARD_DRIVER_DIR)/gpio/gpio.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/gpio
endif

# 根据配置加入 StartySky T1 Timer0 驱动。
ifdef CONFIG_DRIVER_TIMER
SRC_PATH += $(BOARD_DRIVER_DIR)/timer/timer.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/timer
endif

# 根据配置加入 StartySky T1 I2C0 驱动。
ifdef CONFIG_DRIVER_I2C
SRC_PATH += $(BOARD_DRIVER_DIR)/i2c/i2c.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/i2c
endif

# 根据配置加入 StartySky T1 QSPI0 驱动。
ifdef CONFIG_DRIVER_QSPI
SRC_PATH += $(BOARD_DRIVER_DIR)/qspi/qspi.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/qspi
endif
