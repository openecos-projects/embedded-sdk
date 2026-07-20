.PHONY: all clean

BUILD_DIR = ./build

GCC = riscv64-unknown-elf-
AS = $(GCC)as
LD = $(GCC)ld
OBJDUMP = $(GCC)objdump
OBJCOPY = $(GCC)objcopy
LDFLAGS += -T flash.ld
APP ?= main
APP_ALL = $(APP)-asm
OUTPUT_BASE = $(BUILD_DIR)/$(APP_ALL)

MEM_REPORT_OBJDUMP := $(OBJDUMP)
include $(ECOS_SDK_HOME)/tools/scripts/mem_report.mk

all: $(APP).s flash.ld
	@mkdir -p $(BUILD_DIR)
	@$(AS) -mabi=ilp32 -march=rv32im $(APP).s -o $(OUTPUT_BASE).o
	@$(LD) -m elf32lriscv $(LDFLAGS) $(OUTPUT_BASE).o -o $(OUTPUT_BASE).elf
	@$(OBJDUMP) -d $(OUTPUT_BASE).elf > $(OUTPUT_BASE).txt
	@$(OBJCOPY) -S -O binary $(OUTPUT_BASE).elf $(OUTPUT_BASE).bin
	$(call show_mem_usage,$(OUTPUT_BASE).elf)

clean:
	@rm -rf $(BUILD_DIR)
