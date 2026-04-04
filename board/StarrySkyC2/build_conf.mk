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

# 可选的驱动列表
define driver_template
ifdef CONFIG_DRIVER_$(1)
        SRC_PATH += $(shell find $(ECOS_SDK_HOME)/board/StarrySkyC2/driver/$(2) -name "*.c")
        CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/hal/$(2) -type d))
endif
endef
DRIVER_DIR := $(ECOS_SDK_HOME)/board/StarrySkyC2/driver
DRIVER_SUBDIRS := $(notdir $(shell find $(DRIVER_DIR) -mindepth 1 -maxdepth 1 -type d))

$(foreach subdir,$(DRIVER_SUBDIRS), \
    $(eval CONFIG_NAME := $(shell echo $(subdir) | tr '[:lower:]' '[:upper:]')) \
    $(eval $(call driver_template,$(CONFIG_NAME),$(subdir))) \
)

# 可选的链接库列表
ifdef CONFIG_LINK_LIBC
SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libc/src -name "*.c")
CFLAGS += -I$(ECOS_SDK_HOME)/components/libc/include
endif

ifdef CONFIG_LINK_LIBGCC
SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libgcc/src -name "*.[cS]")
CFLAGS += -I$(ECOS_SDK_HOME)/components/libgcc/include
endif
