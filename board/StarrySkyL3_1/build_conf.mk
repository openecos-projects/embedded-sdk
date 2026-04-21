# 根据配置文件设置链接选项
ifdef CONFIG_LINK_TARGET_XIP
LDFLAGS += -DCONFIG_LINK_TARGET_XIP
CFLAGS += -DCONFIG_LINK_TARGET_XIP
endif

ifdef CONFIG_LINK_TARGET_MEM
LDFLAGS += -DCONFIG_LINK_TARGET_MEM
CFLAGS += -DCONFIG_LINK_TARGET_MEM
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

# 可选的驱动列表
define driver_template
ifdef CONFIG_DRIVER_$(1)
        SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/board/StarrySkyL3_1/driver/$(2) -name "*.c")
        CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/hal/$(2) -type d))
endif
endef
DRIVER_DIR := $(ECOS_SDK_HOME)/board/StarrySkyL3_1/driver
DRIVER_SUBDIRS := $(notdir $(shell find $(DRIVER_DIR) -mindepth 1 -maxdepth 1 -type d 2>/dev/null))

$(foreach subdir,$(DRIVER_SUBDIRS), \
    $(eval CONFIG_NAME := $(shell echo $(subdir) | tr '[:lower:]' '[:upper:]')) \
	$(if $(VERBOSE),$(info Include drivers: $(CONFIG_NAME))) \
    $(eval $(call driver_template,$(CONFIG_NAME),$(subdir))) \
)

# 可选的链接库列表
ifdef CONFIG_LINK_LIBC
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libc/src -name "*.c")
CFLAGS += -I$(ECOS_SDK_HOME)/components/libc/include
endif

ifdef CONFIG_LINK_LIBGCC
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libgcc/src -name "*.[cS]")
CFLAGS += -I$(ECOS_SDK_HOME)/components/libgcc/include
endif

# 自动包含所有的 devices 组件的头文件（方便代码补全）
CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/devices/*/include -type d 2>/dev/null))

ifdef CONFIG_DEVICE_ST7735
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/devices/st7735/src -name "*.c")
endif

ifdef CONFIG_DEVICE_ST7789
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/devices/st7789/src -name "*.c")
endif

ifdef CONFIG_DEVICE_SGP30
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/devices/gy_sgp30/src -name "*.c")
endif

ifdef CONFIG_DEVICE_PCF8563
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/devices/pcf8563/src -name "*.c")
endif

