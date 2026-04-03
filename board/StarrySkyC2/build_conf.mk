# 根据配置文件设置链接选项
ifdef CONFIG_LINK_RAM_REGION_SRAM
	LDFLAGS += -DCONFIG_LINK_RAM_REGION_SRAM
	CFLAGS += -DCONFIG_LINK_RAM_REGION_SRAM
endif

ifdef CONFIG_LINK_RAM_REGION_PSRAM
	LDFLAGS += -DCONFIG_LINK_RAM_REGION_PSRAM
	CFLAGS += -DCONFIG_LINK_RAM_REGION_PSRAM
endif

# 根据配置文件添加编译优化选项
ifdef CONFIG_BUILD_OPT_FLAGS
	CFLAGS += $(subst ",,$(CONFIG_BUILD_OPT_FLAGS))
endif

# 根据配置文件添加调试选项
ifdef CONFIG_BUILD_DEBUG
	CFLAGS += -g -DDEBUG
endif

# 根据配置文件添加详细输出选项
ifdef CONFIG_BUILD_VERBOSE
	VERBOSE := 1
endif

# 固件名称 - 从配置文件读取，如果未定义则使用默认值
ifdef CONFIG_FIRMWARE_NAME
	FIRMWARE_NAME := $(subst ",,$(CONFIG_FIRMWARE_NAME))
else
	FIRMWARE_NAME := main
endif

# 可选的源文件列表
ifdef CONFIG_DRIVER_GPIO
	SRC_PATH += $(shell find $(ECOS_SDK_HOME)/board/StarrySkyC2/driver/gpio -name "*.c")
	CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/hal/gpio -type d))
endif

ifdef CONFIG_DRIVER_SYS_UART
	SRC_PATH += $(shell find $(ECOS_SDK_HOME)/board/StarrySkyC2/driver/sys_uart -name "*.c")
	CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/hal/sys_uart -type d))
endif

ifdef CONFIG_DRIVER_TIMER
	SRC_PATH += $(shell find $(ECOS_SDK_HOME)/board/StarrySkyC2/driver/timer -name "*.c")
	CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/hal/timer -type d))
endif

ifdef CONFIG_LINK_LIBC
	SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libc/src -name "*.c")
	CFLAGS += -I$(ECOS_SDK_HOME)/components/libc/include
endif

ifdef CONFIG_LINK_LIBGCC
	SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libgcc/src -name "*.[cS]")
	CFLAGS += -I$(ECOS_SDK_HOME)/components/libgcc/include
endif
