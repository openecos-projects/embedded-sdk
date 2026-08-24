# StarrySkyL4 firmware build script.
# Keep the L4 C-project build behavior aligned with verif/:
#   1. compile every .c/.S source into an object file;
#   2. link objects with riscv-none-elf-ld and the PSRAM linker script;
#   3. generate objdump and stripped binary with objcopy -S -O binary.

ifeq ($(YSYX_AM_BUILD),1)

# ysyx-am 专用分支只负责受控 AM 工程，不改变普通 ECOS 构建规则。
CROSS_COMPILE ?= riscv-none-elf-
YSYX_ARCH := riscv32e-ysyxsoc
YSYX_AM_HAL_SRCS := $(ECOS_SDK_HOME)/board/StarrySkyL4/driver/sys_uart/sys_uart.c \
	$(ECOS_SDK_HOME)/board/StarrySkyL4/driver/timer/timer.c \
	$(ECOS_SDK_HOME)/board/StarrySkyL4/driver/qspi/qspi.c

ifeq ($(strip $(YSYX_STAGE_DIR)),)
$(error 错误：缺少 YSYX_STAGE_DIR)
endif
ifeq ($(strip $(YSYX_BUILD_ROOT)),)
$(error 错误：缺少 YSYX_BUILD_ROOT)
endif
ifeq ($(strip $(YSYX_IMAGE_DIR)),)
$(error 错误：缺少 YSYX_IMAGE_DIR)
endif
YSYX_MICROBENCH_MAINARGS := test train ref huge
ifneq ($(strip $(YSYX_MAINARGS)),)
ifneq ($(YSYX_PROGRAM_ID),benchmarks/microbench)
$(error 错误：只有 benchmarks/microbench 支持 MAINARGS)
endif
ifneq ($(words $(strip $(YSYX_MAINARGS))),1)
$(error 错误：microbench MAINARGS 必须是单个规模名称)
endif
ifeq ($(filter $(strip $(YSYX_MAINARGS)),$(YSYX_MICROBENCH_MAINARGS)),)
$(error 错误：microbench MAINARGS 只允许 test/train/ref/huge)
endif
endif

.DEFAULT_GOAL := ysyx-am

ysyx-am:
	@mkdir -p "$(YSYX_GENERATED_DIR)/generated" "$(YSYX_IMAGE_DIR)" "$(YSYX_BUILD_ROOT)"
	@{ \
		echo '#ifndef __YSYX_AM_AUTOCONF_H__'; \
		echo '#define __YSYX_AM_AUTOCONF_H__'; \
		echo '#define CONFIG_CPU_FREQ_MHZ $(YSYX_CPU_FREQ_MHZ)'; \
		echo '#define CONFIG_TIMER_FREQ_MHZ $(YSYX_TIMER_FREQ_MHZ)'; \
		echo '#define CONFIG_UART_BAUD_RATE 115200'; \
		echo '#define CONFIG_MAINARGS "$(strip $(YSYX_MAINARGS))"'; \
		echo '#define CONFIG_YSYX_AM 1'; \
		echo '#endif'; \
	} > "$(YSYX_GENERATED_DIR)/generated/autoconf.h"
	@$(MAKE) --no-print-directory -C "$(YSYX_STAGE_DIR)" image \
		AM_HOME="$(AM_HOME)" \
		ARCH="$(YSYX_ARCH)" \
		BUILD_ROOT="$(YSYX_BUILD_ROOT)" \
		IMAGE_OUTPUT_DIR="$(YSYX_IMAGE_DIR)" \
		IMAGE_NAME=retrosoc_fw \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		ECOS_SDK_HOME="$(ECOS_SDK_HOME)" \
		YSYX_L4=1 \
		YSYX_BOARD_ID="$(YSYX_BOARD_ID)" \
		YSYX_PROGRAM_ID="$(YSYX_PROGRAM_ID)" \
		YSYX_APP_PATH="$(YSYX_APP_PATH)" \
		YSYX_PROGRAM_AUTHORIZED="$(YSYX_PROGRAM_AUTHORIZED)" \
		YSYX_CORE_MARCH="$(YSYX_CORE_MARCH)" \
		YSYX_CORE_MABI="$(YSYX_CORE_MABI)" \
		YSYX_GENERATED_DIR="$(YSYX_GENERATED_DIR)" \
		YSYX_AM_HAL_SRCS="$(YSYX_AM_HAL_SRCS)"

.PHONY: ysyx-am

else

TOOL_PATH ?= /home/dallous/share/project/darch/tools/builder/target/bin
GCC_M ?= $(TOOL_PATH)/riscv-none-elf-gcc
LD_M ?= $(TOOL_PATH)/riscv-none-elf-ld
OBJDUMP_M ?= $(TOOL_PATH)/riscv-none-elf-objdump
OBJCOPY_M ?= $(TOOL_PATH)/riscv-none-elf-objcopy

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
PROJECT_PATH := $(shell pwd)
CATEGORY := StarrySkyL4

# Include generated Kconfig values.
-include configs/config/auto.conf

# ysyx8c2 cores are RV32E: 16 GPRs and no M. Use ilp32e so GCC
# never emits x16-x31 or hardware mul/div.
VERIF_CFLAGS := -Wall \
	  -std=gnu11 \
	  -march=rv32e \
	  -mabi=ilp32e \
	  -nostdlib \
	  -fdata-sections \
	  -ffunction-sections
CFLAGS := $(VERIF_CFLAGS)

# Add board drivers/components selected by Kconfig.
SDK_SRC_PATH :=
include $(ECOS_SDK_HOME)/board/StarrySkyL4/build_conf.mk

ifndef FIRMWARE_NAME
FIRMWARE_NAME := main
endif
.DEFAULT_GOAL := $(FIRMWARE_NAME)

STARTUP_SRC := ./start.S
BOOTLOADER_SRC := ./loader/loader.c
APP_SRC := ./main.c $(SDK_SRC_PATH)
STARTUP_OBJ := $(OBJ_DIR)/$(STARTUP_SRC:.S=.o)
BOOTLOADER_OBJ := $(OBJ_DIR)/$(BOOTLOADER_SRC:.c=.o)
APP_OBJS := $(addprefix $(OBJ_DIR)/,$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(APP_SRC))))
FIRMWARE_OBJS := $(STARTUP_OBJ) $(BOOTLOADER_OBJ) $(APP_OBJS)

CFLAGS += -I./ -I./configs

# The verified L4 QSPI polling sequence is timing-sensitive at -Os/-O2.
QSPI_SRCS := $(filter %/qspi.c,$(APP_SRC))
QSPI_OBJS := $(addprefix $(OBJ_DIR)/,$(QSPI_SRCS:.c=.o))
$(QSPI_OBJS): CFLAGS := $(filter-out -O%,$(CFLAGS)) -O0

FIRMWARE_LDS_PATH := ./sections.lds
LDFLAGS := -e _start -melf32lriscv -z noexecstack -T $(FIRMWARE_LDS_PATH) --gc-sections

# Keep SDK memory report output after the verif-style link flow.
MEM_REPORT_CODE_IN_RAM := 1
-include $(ECOS_SDK_HOME)/tools/scripts/mem_report.mk

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(GCC_M) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(GCC_M) $(CFLAGS) -c -o $@ $<

# The FSBL runs before PSRAM contains .text. Compile it exactly like verif so
# GCC cannot turn its copy loops into calls to memcpy in the unloaded .text.
$(BOOTLOADER_OBJ): $(BOOTLOADER_SRC) $(STARTUP_OBJ)
	@mkdir -p $(dir $@)
	@$(GCC_M) $(VERIF_CFLAGS) -c -o $@ $<

$(APP_OBJS): | $(BOOTLOADER_OBJ)

$(FIRMWARE_NAME): $(FIRMWARE_OBJS)
	@mkdir -p $(BUILD_DIR)
	@$(LD_M) $(LDFLAGS) -o $(BUILD_DIR)/$(FIRMWARE_NAME).elf --start-group $(FIRMWARE_OBJS) --end-group
	@$(OBJDUMP_M) -d $(BUILD_DIR)/$(FIRMWARE_NAME).elf > $(BUILD_DIR)/$(FIRMWARE_NAME).txt
	@$(OBJCOPY_M) -S -O binary $(BUILD_DIR)/$(FIRMWARE_NAME).elf $(BUILD_DIR)/$(FIRMWARE_NAME).bin
	@$(OBJCOPY_M) --change-addresses -0x30000000 -O verilog $(BUILD_DIR)/$(FIRMWARE_NAME).elf $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	$(call show_mem_usage, $(BUILD_DIR)/$(FIRMWARE_NAME).elf)

clean:
	@rm -rf $(BUILD_DIR)

clean_config:
	@rm -rf configs/generated configs/config configs/.config configs/.config.old

clean_all:
	@rm -rf $(BUILD_DIR)
	@rm -rf configs/generated configs/config configs/.config configs/.config.old
	@rm -rf $(PROJECT_PATH)/tools/fixdep/build
	@rm -rf $(PROJECT_PATH)/tools/kconfig/build

-include scripts/config.mk

.PHONY: $(FIRMWARE_NAME) clean clean_config clean_all

endif
