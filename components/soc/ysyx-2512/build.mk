# ysyx-2512 firmware build rules.

ifndef ECOS_SDK_HOME
$(error ECOS_SDK_HOME is not set)
endif
ifndef PROJECT_DIR
$(error PROJECT_DIR is not set)
endif

SOC_ROOT := $(ECOS_SDK_HOME)/components/soc/ysyx-2512
BUILD_DIR := $(PROJECT_DIR)/build
OBJ_DIR := $(BUILD_DIR)/obj
FIRMWARE_NAME ?= retrosoc_fw

CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
LD := $(CROSS)ld
OBJDUMP := $(CROSS)objdump
OBJCOPY := $(CROSS)objcopy

BASE_CFLAGS := -Wall \
	-std=gnu11 \
	-march=rv32e \
	-mabi=ilp32e \
	-nostdlib \
	-fdata-sections \
	-ffunction-sections
APP_CFLAGS := $(BASE_CFLAGS) -Os -g \
	-I$(PROJECT_DIR) \
	-I$(SOC_ROOT)/include \
	-I$(ECOS_SDK_HOME)/hal/sys_uart
BOOT_CFLAGS := $(BASE_CFLAGS)
LDFLAGS := -e _start -melf32lriscv -z noexecstack \
	-T $(SOC_ROOT)/linker/sections.lds --gc-sections

STARTUP_OBJ := $(OBJ_DIR)/startup.o
LOADER_OBJ := $(OBJ_DIR)/loader.o
APP_OBJ := $(OBJ_DIR)/main.o
SYS_UART_OBJ := $(OBJ_DIR)/sys_uart.o
FIRMWARE_OBJS := $(STARTUP_OBJ) $(LOADER_OBJ) $(APP_OBJ) $(SYS_UART_OBJ)
ELF := $(BUILD_DIR)/$(FIRMWARE_NAME).elf
BIN := $(BUILD_DIR)/$(FIRMWARE_NAME).bin
TXT := $(BUILD_DIR)/$(FIRMWARE_NAME).txt
HEX := $(BUILD_DIR)/$(FIRMWARE_NAME).hex

.DEFAULT_GOAL := all

all: $(ELF) $(BIN) $(TXT) $(HEX)

$(STARTUP_OBJ): $(SOC_ROOT)/startup/start.S
	@mkdir -p $(dir $@)
	@$(CC) $(APP_CFLAGS) -c -o $@ $<

$(LOADER_OBJ): $(SOC_ROOT)/startup/loader.c $(STARTUP_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(BOOT_CFLAGS) -c -o $@ $<

$(APP_OBJ): $(PROJECT_DIR)/main.c $(PROJECT_DIR)/main.h | $(LOADER_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(APP_CFLAGS) -c -o $@ $<

$(SYS_UART_OBJ): $(SOC_ROOT)/hal/sys_uart/sys_uart.c \
	$(SOC_ROOT)/include/ysyx_2512_soc.h \
	$(ECOS_SDK_HOME)/hal/sys_uart/hal_sys_uart.h | $(LOADER_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(APP_CFLAGS) -c -o $@ $<

$(ELF): $(FIRMWARE_OBJS) $(SOC_ROOT)/linker/sections.lds
	@mkdir -p $(BUILD_DIR)
	@$(LD) $(LDFLAGS) -o $@ --start-group $(FIRMWARE_OBJS) --end-group

$(BIN): $(ELF)
	@$(OBJCOPY) -S -O binary $< $@

$(TXT): $(ELF)
	@$(OBJDUMP) -d $< > $@

$(HEX): $(ELF)
	@$(OBJCOPY) --change-addresses -0x30000000 -O verilog $< $@

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
