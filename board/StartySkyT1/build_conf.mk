# 根据配置加入 StartySky T1 SDRAM 启动加载器和链接宏。
ifdef CONFIG_LINK_TARGET_SDRAM
SRC_PATH += ./loader/loader.c
CFLAGS += -DCONFIG_LINK_TARGET_SDRAM=1
LINKER_CPPFLAGS += -DCONFIG_LINK_TARGET_SDRAM=1
endif

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

# 根据配置加入 StartySky T1 机器中断和 PLIC 驱动。
ifdef CONFIG_DRIVER_INTERRUPT
SRC_PATH += $(BOARD_DRIVER_DIR)/interrupt/interrupt.c
SRC_PATH += $(BOARD_DRIVER_DIR)/interrupt/trap.S
CFLAGS += -I$(ECOS_SDK_HOME)/hal/interrupt
endif

# 根据配置加入 StartySky T1 GPIO 外部中断驱动。
ifdef CONFIG_DRIVER_GPIO_INTERRUPT
SRC_PATH += $(BOARD_DRIVER_DIR)/gpio/gpio_interrupt.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/interrupt
endif

# 根据配置加入 StartySky T1 Timer0 驱动。
ifdef CONFIG_DRIVER_TIMER
SRC_PATH += $(BOARD_DRIVER_DIR)/timer/timer.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/timer
endif

# 根据配置加入 StartySky T1 Timer0 中断驱动。
ifdef CONFIG_DRIVER_TIMER_INTERRUPT
SRC_PATH += $(BOARD_DRIVER_DIR)/timer/timer_interrupt.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/interrupt
endif

# 根据配置加入 StartySky T1 CLINT 驱动。
ifdef CONFIG_DRIVER_CLINT
SRC_PATH += $(BOARD_DRIVER_DIR)/clint/clint.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/clint
endif

# 根据配置加入 StartySky T1 SDRAM 驱动。
ifdef CONFIG_DRIVER_SDRAM
SRC_PATH += $(BOARD_DRIVER_DIR)/sdram/sdram.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/sdram
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
