.PHONY: all

BUILD_DIR = ./build

GCC = riscv64-unknown-elf-
AS = $(GCC)as
LD = $(GCC)ld
OBJDUMP = $(GCC)objdump
OBJCOPY = $(GCC)objcopy
LDFLAGS += -T flash.ld
APP ?= main
APP_ALL = $(APP)-asm

all: $(APP).s
	@mkdir -p $(BUILD_DIR)/flash
	@mkdir -p ../../bin/flash
	$(AS) -mabi=ilp32 -march=rv32im $(APP).s -o $(APP_ALL).o
	$(LD) -m elf32lriscv $(LDFLAGS) $(APP_ALL).o -o $(APP_ALL).elf
	$(OBJDUMP) -d $(APP_ALL).elf > $(APP_ALL).txt
	$(OBJCOPY) -S -O binary $(APP_ALL).elf $(APP_ALL).bin
	@mv $(APP_ALL).bin $(BUILD_DIR)/$(APP_ALL).bin
	@mv $(APP_ALL).txt $(BUILD_DIR)/$(APP_ALL).txt
	@mv $(APP_ALL).elf $(BUILD_DIR)/$(APP_ALL).elf
	@mv $(APP_ALL).o $(BUILD_DIR)/$(APP_ALL).o
	@echo "Build $(APP_ALL) done!"

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.elf
