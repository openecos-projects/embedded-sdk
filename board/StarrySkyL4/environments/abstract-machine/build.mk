# StarrySkyL4 integration entry for the AbstractMachine environment.
CROSS_COMPILE ?= riscv64-unknown-elf-
AM_ARCH := riscv32e-ysyxsoc
AM_PORT_DIR := $(ECOS_SDK_HOME)/board/StarrySkyL4/environments/abstract-machine
AM_PLATFORM_MK := $(AM_PORT_DIR)/platform.mk
AM_HAL_SRCS := $(ECOS_SDK_HOME)/board/StarrySkyL4/driver/sys_uart/sys_uart.c \
	$(ECOS_SDK_HOME)/board/StarrySkyL4/driver/timer/timer.c \
	$(ECOS_SDK_HOME)/board/StarrySkyL4/driver/qspi/qspi.c

ifeq ($(strip $(AM_STAGE_DIR)),)
$(error 错误：缺少 AM_STAGE_DIR)
endif
ifeq ($(strip $(AM_BUILD_ROOT)),)
$(error 错误：缺少 AM_BUILD_ROOT)
endif
ifeq ($(strip $(AM_IMAGE_DIR)),)
$(error 错误：缺少 AM_IMAGE_DIR)
endif

AM_MICROBENCH_MAINARGS := test train ref huge
ifneq ($(strip $(AM_MAINARGS)),)
ifneq ($(AM_PROGRAM_ID),benchmarks/microbench)
$(error 错误：只有 benchmarks/microbench 支持 MAINARGS)
endif
ifneq ($(words $(strip $(AM_MAINARGS))),1)
$(error 错误：microbench MAINARGS 必须是单个规模名称)
endif
ifeq ($(filter $(strip $(AM_MAINARGS)),$(AM_MICROBENCH_MAINARGS)),)
$(error 错误：microbench MAINARGS 只允许 test/train/ref/huge)
endif
endif

.DEFAULT_GOAL := abstract-machine

abstract-machine:
	@mkdir -p "$(AM_GENERATED_DIR)/generated" "$(AM_IMAGE_DIR)" "$(AM_BUILD_ROOT)"
	@{ \
		echo '#ifndef __ECOS_AM_AUTOCONF_H__'; \
		echo '#define __ECOS_AM_AUTOCONF_H__'; \
		echo '#define CONFIG_CPU_FREQ_MHZ $(AM_CPU_FREQ_MHZ)'; \
		echo '#define CONFIG_TIMER_FREQ_MHZ $(AM_TIMER_FREQ_MHZ)'; \
		echo '#define CONFIG_UART_BAUD_RATE 115200'; \
		echo '#define CONFIG_MAINARGS "$(strip $(AM_MAINARGS))"'; \
		echo '#define CONFIG_ABSTRACT_MACHINE 1'; \
		echo '#endif'; \
	} > "$(AM_GENERATED_DIR)/generated/autoconf.h"
	@$(MAKE) --no-print-directory -C "$(AM_STAGE_DIR)" image \
		AM_HOME="$(AM_HOME)" \
		ARCH="$(AM_ARCH)" \
		BUILD_ROOT="$(AM_BUILD_ROOT)" \
		IMAGE_OUTPUT_DIR="$(AM_IMAGE_DIR)" \
		IMAGE_NAME=retrosoc_fw \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		ECOS_SDK_HOME="$(ECOS_SDK_HOME)" \
		ECOS_AM_BUILD=1 \
		AM_BOARD_ID="$(AM_BOARD_ID)" \
		AM_PROGRAM_ID="$(AM_PROGRAM_ID)" \
		AM_APP_PATH="$(AM_APP_PATH)" \
		AM_PROGRAM_AUTHORIZED="$(AM_PROGRAM_AUTHORIZED)" \
		AM_CORE_MARCH="$(AM_CORE_MARCH)" \
		AM_CORE_MABI="$(AM_CORE_MABI)" \
		AM_GENERATED_DIR="$(AM_GENERATED_DIR)" \
		AM_PORT_DIR="$(AM_PORT_DIR)" \
		AM_PLATFORM_MK="$(AM_PLATFORM_MK)" \
		AM_HAL_SRCS="$(AM_HAL_SRCS)"

.PHONY: abstract-machine
