# retroSoC 固件构建脚本
# 用于编译基于RISC-V架构的嵌入式固件

CROSS=riscv64-unknown-elf-
BUILD_DIR := build
PROJECT_PATH := $(shell pwd)
CATEGORY := StarrySkyL3_1

# 包含配置文件
-include configs/config/auto.conf

CFLAGS := -mabi=lp64 \
	  -march=rv64ifd_zifencei \
	  -mcmodel=medany \
	  -Wl,-Bstatic,--strip-debug \
	  -ffreestanding \
	  -nostdlib

# 包含可选的设置 (从 Kconfig 生成并包含驱动)
SDK_SRC_PATH :=
include $(ECOS_SDK_HOME)/board/StarrySkyL3_1/build_conf.mk

# 默认兼容，如果没选驱动或组件
# CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/components -type d))

# Firmware Sources
FIRMWARE_SRC_PATH := ./start.s ./main.c $(SDK_SRC_PATH)

# loader Sources
LOADER_SRC_PATH := loader/start.s loader/loader.c loader/payload.S $(SDK_SRC_PATH)

CFLAGS += -I./configs 
		
# 链接脚本路径
FIRMWARE_LDS_PATH := ./sections.lds
LOADER_LDS_PATH := ./loader/loader.lds

# 包含内存报告工具
-include $(ECOS_SDK_HOME)/tools/scripts/mem_report.mk

# 主要构建目标：生成固件的所有格式文件
$(FIRMWARE_NAME):
	@echo "Building APP $(FIRMWARE_NAME)..."
	@mkdir -p $(BUILD_DIR)

	@echo "Preprocess APP linker script..."
	@$(CROSS)cpp -P $(CFLAGS) -o $(BUILD_DIR)/app.lds $(FIRMWARE_LDS_PATH)
	@echo "Compiling APP..."
	@$(CROSS)gcc $(CFLAGS) -I./ -T $(BUILD_DIR)/app.lds -o $(BUILD_DIR)/app.elf $(FIRMWARE_SRC_PATH)
	@$(CROSS)objcopy -O binary  $(BUILD_DIR)/app.elf $(BUILD_DIR)/app.bin

ifdef CONFIG_LINK_TARGET_FLASH
	@echo "Skipping Bootloader (Target is FLASH)..."
	@$(CROSS)objcopy -O verilog $(BUILD_DIR)/app.elf $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	@sed -i 's/@30000000/@00000000/g' $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	@$(CROSS)objcopy -O binary  $(BUILD_DIR)/app.elf $(BUILD_DIR)/$(FIRMWARE_NAME).bin
	@$(CROSS)objdump -d $(BUILD_DIR)/app.elf > $(BUILD_DIR)/$(FIRMWARE_NAME).txt
	$(call show_mem_usage, $(BUILD_DIR)/app.elf)
else
	@echo "Building Bootloader..."
	@$(CROSS)cpp -P -o $(BUILD_DIR)/loader.lds $(LOADER_LDS_PATH)
	@$(CROSS)gcc $(CFLAGS) -I./ -T $(BUILD_DIR)/loader.lds -o $(BUILD_DIR)/loader.elf $(LOADER_SRC_PATH)

	@echo "Final linking ..."
	@$(CROSS)objcopy -O verilog $(BUILD_DIR)/loader.elf $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	@sed -i 's/@30000000/@00000000/g' $(BUILD_DIR)/$(FIRMWARE_NAME).hex

	@$(CROSS)objcopy -O binary  $(BUILD_DIR)/loader.elf $(BUILD_DIR)/$(FIRMWARE_NAME).bin
	@$(CROSS)objdump -d $(BUILD_DIR)/loader.elf > $(BUILD_DIR)/$(FIRMWARE_NAME).txt
	$(call show_mem_usage, $(BUILD_DIR)/loader.elf)
endif
	@echo "Done."

# 清理构建产物
clean:
	rm -rf $(BUILD_DIR)

clean_config:
	rm -rf configs/generated configs/config configs/.config configs/.config.old
	
clean_all:
	rm -rf $(BUILD_DIR) 
	rm -rf configs/generated configs/config configs/.config configs/.config.old
	rm -rf $(PROJECT_PATH)/tools/fixdep/build
	rm -rf $(PROJECT_PATH)/tools/kconfig/build

-include scripts/config.mk
# 声明伪目标
.PHONY: $(FIRMWARE_NAME) clean
