.DEFAULT_GOAL := all

BUILD_DIR := build
CROSS ?= riscv64-unknown-elf-

AS := $(CROSS)as
LD := $(CROSS)ld
OBJDUMP := $(CROSS)objdump
OBJCOPY := $(CROSS)objcopy

APP ?= main
SOURCE := $(APP).s
OUTPUT_BASE := $(BUILD_DIR)/$(APP)-asm

# 配置 StartySky T1 的汇编架构和 XFlash 链接参数。
ASFLAGS := -march=rv32imc_zicsr_zifencei -mabi=ilp32
LDFLAGS := -m elf32lriscv -T flash.ld -z noexecstack \
	--gc-sections --build-id=none --no-relax \
	-Map=$(OUTPUT_BASE).map

# 生成可写入 XFlash 的 ELF、BIN 和反汇编文本。
all: $(OUTPUT_BASE).elf
	@$(OBJDUMP) -d $(OUTPUT_BASE).elf > $(OUTPUT_BASE).txt
	@$(OBJCOPY) -S -O binary $(OUTPUT_BASE).elf $(OUTPUT_BASE).bin

# 将汇编源文件编译为目标文件。
$(OUTPUT_BASE).o: $(SOURCE) Makefile
	@mkdir -p $(BUILD_DIR)
	@$(AS) $(ASFLAGS) $(SOURCE) -o $(OUTPUT_BASE).o

# 将目标文件链接到 XFlash 地址空间。
$(OUTPUT_BASE).elf: $(OUTPUT_BASE).o flash.ld
	@$(LD) $(LDFLAGS) $(OUTPUT_BASE).o -o $(OUTPUT_BASE).elf

# 删除当前汇编工程的构建产物。
clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
