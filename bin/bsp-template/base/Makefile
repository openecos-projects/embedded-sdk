# @BSP_NAME@ 普通工程构建文件。
# [不要修改] 项目生成器会创建这两个配置文件。
-include configs/sdk-path.mk
-include configs/board-package.mk
-include configs/config/auto.conf

BUILD_DIR := build
PROJECT_PATH := $(shell pwd)
CATEGORY := @BSP_CATEGORY@

# [必须修改] TODO_BSP_REQUIRED：确认工具链前缀。
CROSS ?= riscv64-unknown-elf-

# [必须修改] TODO_BSP_REQUIRED：根据处理器手册确认 -march 和 -mabi。
CFLAGS := -Wall -std=gnu11 -march=rv32im -mabi=ilp32 \
	-ffreestanding -nostdlib -fdata-sections -ffunction-sections

ifdef CONFIG_BUILD_OPT_FLAGS
CFLAGS += $(subst ",,$(CONFIG_BUILD_OPT_FLAGS))
endif

ifdef CONFIG_BUILD_DEBUG
CFLAGS += -g -DDEBUG
endif

ifdef CONFIG_FIRMWARE_NAME
FIRMWARE_NAME := $(subst ",,$(CONFIG_FIRMWARE_NAME))
else
FIRMWARE_NAME := main
endif

# [不要修改] 普通工程入口文件由模板和 BSP 注入。
SRC_PATH := ./start.S ./main.c
CFLAGS += -I./ -I./configs

# [不要修改] 外部驱动和 SDK 组件由 build_conf.mk 加入。
include $(BOARD_BUILD_CONF)

# [必须修改] TODO_BSP_REQUIRED：确认链接参数和入口符号与 sections.lds 一致。
LDFLAGS := -T ./sections.lds -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(FIRMWARE_NAME).map

.DEFAULT_GOAL := $(FIRMWARE_NAME)

$(FIRMWARE_NAME):
	@mkdir -p $(BUILD_DIR)
	@$(CROSS)gcc $(CFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$(FIRMWARE_NAME).elf $(SRC_PATH)
	@$(CROSS)objcopy -O binary $(BUILD_DIR)/$(FIRMWARE_NAME).elf $(BUILD_DIR)/$(FIRMWARE_NAME).bin
	@$(CROSS)objcopy -O verilog $(BUILD_DIR)/$(FIRMWARE_NAME).elf $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	@$(CROSS)objdump -d $(BUILD_DIR)/$(FIRMWARE_NAME).elf > $(BUILD_DIR)/$(FIRMWARE_NAME).txt

clean:
	@rm -rf $(BUILD_DIR)

clean_config:
	@rm -rf configs/generated configs/config configs/.config configs/.config.old

-include scripts/config.mk

.PHONY: $(FIRMWARE_NAME) clean clean_config
