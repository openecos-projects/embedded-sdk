# @BSP_NAME@ 驱动和 SDK 组件选择。
# [不要修改] BOARD_DRIVER_DIR 和 ECOS_SDK_HOME 由 SDK 提供。

ifdef CONFIG_DRIVER_SYS_UART
SRC_PATH += $(BOARD_DRIVER_DIR)/sys_uart/sys_uart.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/sys_uart
endif

ifdef CONFIG_DRIVER_GPIO
SRC_PATH += $(BOARD_DRIVER_DIR)/gpio/gpio.c
CFLAGS += -I$(ECOS_SDK_HOME)/hal/gpio
endif

# [按需修改] 新增驱动时，同时修改 driver.kconfig 和本文件。

ifdef CONFIG_LINK_LIBC
SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libc/src -name "*.c")
CFLAGS += -I$(ECOS_SDK_HOME)/components/libc/include
endif

ifdef CONFIG_LINK_LIBGCC
SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libgcc/src -name "*.[cS]")
CFLAGS += -I$(ECOS_SDK_HOME)/components/libgcc/include
endif
