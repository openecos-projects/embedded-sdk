# 引入 SDK 生成的板卡和 Kconfig 配置。
-include configs/board-package.mk
-include configs/config/auto.conf

BUILD_DIR := build
CROSS ?= riscv64-unknown-elf-
LINKER_SCRIPT_SOURCE := ./sections.lds
LINKER_SCRIPT := $(BUILD_DIR)/sections.lds

# 配置 StartySky T1-Pico 的处理器架构、ABI 和裸机构建选项。
CFLAGS := -Wall -Wextra -std=gnu11 \
	-march=rv32imc_zicsr_zifencei -mabi=ilp32 \
	-ffreestanding -nostdlib -fno-builtin -msmall-data-limit=0 \
	-fdata-sections -ffunction-sections

# 根据 Kconfig 加入用户选择的优化选项。
ifdef CONFIG_BUILD_OPT_FLAGS
CFLAGS += $(subst ",,$(CONFIG_BUILD_OPT_FLAGS))
endif

# 根据 Kconfig 加入调试信息。
ifdef CONFIG_BUILD_DEBUG
CFLAGS += -g3 -DDEBUG
endif

# 根据 Kconfig 设置固件名称。
ifdef CONFIG_FIRMWARE_NAME
FIRMWARE_NAME := $(subst ",,$(CONFIG_FIRMWARE_NAME))
else
FIRMWARE_NAME := startysky_t1_pico_uart
endif

# 加入启动代码、模板入口和工程头文件目录。
SRC_PATH := ./start.S ./main.c
CFLAGS += -I./ -I./configs

# 加入板卡配置声明的启动组件和外设驱动。
include $(BOARD_BUILD_CONF)

# 配置链接入口、预处理链接脚本、垃圾回收和映射文件。
LDFLAGS := -nostdlib -Wl,-e,_start -T $(LINKER_SCRIPT) \
	-Wl,--gc-sections -Wl,--build-id=none \
	-Wl,-Map=$(BUILD_DIR)/$(FIRMWARE_NAME).map

.DEFAULT_GOAL := $(FIRMWARE_NAME)

# 生成 ELF、BIN 和反汇编文本产物。
$(FIRMWARE_NAME):
	@mkdir -p $(BUILD_DIR)
	@$(CROSS)gcc -E -P -x c $(LINKER_CPPFLAGS) -o $(LINKER_SCRIPT) $(LINKER_SCRIPT_SOURCE)
	@$(CROSS)gcc $(CFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$(FIRMWARE_NAME).elf $(SRC_PATH)
	@$(CROSS)objcopy -O binary $(BUILD_DIR)/$(FIRMWARE_NAME).elf $(BUILD_DIR)/$(FIRMWARE_NAME).bin
	@$(CROSS)objdump -d $(BUILD_DIR)/$(FIRMWARE_NAME).elf > $(BUILD_DIR)/$(FIRMWARE_NAME).txt

# 删除固件构建产物。
clean:
	@rm -rf $(BUILD_DIR)

# 删除 Kconfig 生成内容。
clean_config:
	@rm -rf configs/generated configs/config configs/.config configs/.config.old

-include scripts/config.mk

.PHONY: $(FIRMWARE_NAME) clean clean_config
