# StarrySkyL4 firmware build script.
# Keep the L4 C-project build behavior aligned with verif/:
#   1. compile every .c/.S source into an object file;
#   2. link objects with riscv-none-elf-ld and the PSRAM linker script;
#   3. generate objdump and stripped binary with objcopy -S -O binary.

CROSS ?= riscv-none-elf-
GCC_M ?= $(CROSS)gcc
LD_M ?= $(CROSS)ld
OBJDUMP_M ?= $(CROSS)objdump
OBJCOPY_M ?= $(CROSS)objcopy

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
PROJECT_PATH := $(shell pwd)
CATEGORY := StarrySkyL4
SOC_ROOT := $(ECOS_SDK_HOME)/components/soc/ysyx-2512

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

STARTUP_SRC := $(SOC_ROOT)/startup/start.S
BOOTLOADER_SRC := $(SOC_ROOT)/startup/loader.c
APP_SRC := ./main.c $(SDK_SRC_PATH)
STARTUP_OBJ := $(OBJ_DIR)/$(STARTUP_SRC:.S=.o)
BOOTLOADER_OBJ := $(OBJ_DIR)/$(BOOTLOADER_SRC:.c=.o)
APP_OBJS := $(addprefix $(OBJ_DIR)/,$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(APP_SRC))))
FIRMWARE_OBJS := $(STARTUP_OBJ) $(BOOTLOADER_OBJ) $(APP_OBJS)

CFLAGS += -I./ -I./configs -I$(SOC_ROOT)/include

# The verified L4 QSPI polling sequence is timing-sensitive at -Os/-O2.
QSPI_SRCS := $(filter %/qspi.c,$(APP_SRC))
QSPI_OBJS := $(addprefix $(OBJ_DIR)/,$(QSPI_SRCS:.c=.o))
$(QSPI_OBJS): CFLAGS := $(filter-out -O%,$(CFLAGS)) -O0

FIRMWARE_LDS_PATH := $(SOC_ROOT)/linker/sections.lds
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
