# ECOS AbstractMachine application build adapter.
PROJECT_ROOT := $(abspath $(CURDIR))
AM_BUILD_DIR := $(PROJECT_ROOT)/build

ifeq ($(strip $(NAME)),)
$(error AM application Makefile must define NAME)
endif
ifeq ($(strip $(SRCS)),)
$(error AM application Makefile must define SRCS)
endif
ifeq ($(wildcard $(AM_HOME)/am/include/am.h),)
$(error AM_HOME is invalid: $(AM_HOME))
endif
ifeq ($(wildcard $(AM_BOARD_PORT_DIR)/board.mk),)
$(error BSP does not provide an AbstractMachine environment: $(AM_BOARD_PORT_DIR))
endif

include $(AM_BOARD_PORT_DIR)/board.mk

AM_CORE_PROFILE_FILE := $(AM_BOARD_PORT_DIR)/cores/$(AM_CORE_PROFILE).conf
ifeq ($(wildcard $(AM_CORE_PROFILE_FILE)),)
$(error BSP default core profile does not exist: $(AM_CORE_PROFILE))
endif
include $(AM_CORE_PROFILE_FILE)

ifneq ($(CORE_PROFILE),$(AM_CORE_PROFILE))
$(error Core profile file does not match BSP default core: $(AM_CORE_PROFILE))
endif
ifeq ($(strip $(MARCH)),)
$(error BSP core profile is missing MARCH)
endif
ifeq ($(strip $(MABI)),)
$(error BSP core profile is missing MABI)
endif

include $(AM_BOARD_PORT_DIR)/build.mk

override ARCH := $(AM_ARCH)
override BUILD_ROOT := $(AM_BUILD_DIR)/work
override IMAGE_OUTPUT_DIR := $(AM_BUILD_DIR)
override IMAGE_NAME := retrosoc_fw
override ECOS_AM_BUILD := 1
override AM_BOARD_ID := $(BOARD_ID)
override AM_CORE_MARCH := $(MARCH)
override AM_CORE_MABI := $(MABI)
override AM_GENERATED_DIR := $(AM_BUILD_DIR)
override AM_MAINARGS := $(strip $(MAINARGS))

export AM_HOME ECOS_SDK_HOME ECOS_AM_BUILD ARCH BUILD_ROOT
export CROSS_COMPILE AM_BOARD_ID AM_CORE_MARCH AM_CORE_MABI
export AM_GENERATED_DIR AM_PORT_DIR AM_PLATFORM_MK AM_HAL_SRCS

ifneq ($(findstring ",$(AM_MAINARGS)),)
$(error MAINARGS cannot contain a double quote)
endif

include $(AM_HOME)/Makefile

.PHONY: am-config am-clean-project
am-config:
	@mkdir -p "$(AM_GENERATED_DIR)/generated"
	@{ \
		echo '#ifndef __ECOS_AM_AUTOCONF_H__'; \
		echo '#define __ECOS_AM_AUTOCONF_H__'; \
		echo '#define CONFIG_CPU_FREQ_MHZ $(CPU_FREQ_MHZ)'; \
		echo '#define CONFIG_TIMER_FREQ_MHZ $(TIMER_FREQ_MHZ)'; \
		echo '#define CONFIG_UART_BAUD_RATE 115200'; \
		echo '#define CONFIG_MAINARGS "$(AM_MAINARGS)"'; \
		echo '#define CONFIG_ABSTRACT_MACHINE 1'; \
		echo '#endif'; \
	} > "$(AM_GENERATED_DIR)/generated/autoconf.h.tmp"
	@mv "$(AM_GENERATED_DIR)/generated/autoconf.h.tmp" \
		"$(AM_GENERATED_DIR)/generated/autoconf.h"

$(LIBS): am-config
$(IMAGE).elf: am-config

am-clean-project:
	@test "$(AM_BUILD_DIR)" = "$(PROJECT_ROOT)/build"
	@rm -rf "$(AM_BUILD_DIR)"

clean: am-clean-project
