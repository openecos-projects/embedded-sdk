CROSS=riscv64-unknown-elf-
BUILD_DIR := build
PROJECT_PATH := $(shell pwd)
CATEGORY := StarrySkyL3

-include configs/config/auto.conf

CFLAGS := -mabi=lp64 \
	  -march=rv64ifd_zifencei \
	  -mcmodel=medany \
	  -Wl,-Bstatic,--strip-debug \
	  -Wl,--build-id=none \
	  -ffreestanding \
	  -nostdlib

# 根据配置文件添加编译优化选项
ifdef CONFIG_BUILD_OPT_FLAGS
CFLAGS += $(subst ",,$(CONFIG_BUILD_OPT_FLAGS))
endif

# 根据配置文件添加调试选项
ifdef CONFIG_BUILD_DEBUG
CFLAGS += -g -DDEBUG
endif

# 根据配置文件添加详细输出选项
ifdef CONFIG_BUILD_VERBOSE
VERBOSE := 1
endif

# 固件名称 - 从配置文件读取，如果未定义则使用默认值
ifdef CONFIG_FIRMWARE_NAME
FIRMWARE_NAME := $(subst ",,$(CONFIG_FIRMWARE_NAME))
else
FIRMWARE_NAME := main
endif

# 源文件列表
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/board/StarrySkyL3/driver/sys_uart -name "*.c")
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libgcc -name "*.c")
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libgcc -name "*.S")
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/libc -name "*.c")
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/gpio -name "*.c")
SDK_SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components/i2c -name "*.c")

# Firmware Sources
FIRMWARE_SRC_PATH := ./start.s ./main.c $(SDK_SRC_PATH)

# loader Sources
LOADER_SRC_PATH := loader/start.s loader/loader.c loader/payload.S $(SDK_SRC_PATH)

CFLAGS += -I./configs 
CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/components -type d))
CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/hal/sys_uart -type d))
# CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/devices -type d))

# The L3 linker script uses these symbols for the application entry address.
CFLAGS += -Wl,--defsym,_pmem_start=$(CONFIG_LINK_ADDRESS),--defsym,_entry_offset=0
		
# 链接脚本路径
FIRMWARE_LDS_PATH := ./sections.lds
LOADER_LDS_PATH := ./loader/loader.lds

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
else
	@echo "Building Bootloader..."
	@$(CROSS)cpp -P -o $(BUILD_DIR)/loader.lds $(LOADER_LDS_PATH)
	@$(CROSS)gcc $(CFLAGS) -I./ -T $(BUILD_DIR)/loader.lds -o $(BUILD_DIR)/loader.elf $(LOADER_SRC_PATH)

	@echo "Final linking ..."
	@$(CROSS)objcopy -O verilog $(BUILD_DIR)/loader.elf $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	@sed -i 's/@30000000/@00000000/g' $(BUILD_DIR)/$(FIRMWARE_NAME).hex

	@$(CROSS)objcopy -O binary  $(BUILD_DIR)/loader.elf $(BUILD_DIR)/$(FIRMWARE_NAME).bin
	@$(CROSS)objdump -d $(BUILD_DIR)/loader.elf > $(BUILD_DIR)/$(FIRMWARE_NAME).txt
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
.PHONY: $(FIRMWARE_NAME).elf clean
