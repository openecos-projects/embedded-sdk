.PHONY: all

BUILD_DIR = ./build

GCC = riscv64-linux-gnu-
AS = $(GCC)as
LD = $(GCC)ld
OBJDUMP = $(GCC)objdump
OBJCOPY = $(GCC)objcopy
LDFLAGS += -T flash.ld
APP ?= sdram
APP_ALL = $(APP)-asm

all: $(APP).s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(APP).s -o $(APP_ALL).o
	$(LD) $(LDFLAGS) $(APP_ALL).o -o $(APP_ALL).elf
	$(OBJDUMP) -d $(APP_ALL).elf > $(APP_ALL).txt
	$(OBJCOPY) -S -O binary $(APP_ALL).elf $(APP_ALL).bin
	@mv $(APP_ALL).bin ../../bin/flash/$(APP_ALL).bin
	@mv $(APP_ALL).txt ../../bin/flash/$(APP_ALL).txt
	@mv $(APP_ALL).elf $(BUILD_DIR)/$(APP_ALL).elf
	@mv $(APP_ALL).o $(BUILD_DIR)/$(APP_ALL).o
	@echo "Build $(APP_ALL) done!"

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.elf
