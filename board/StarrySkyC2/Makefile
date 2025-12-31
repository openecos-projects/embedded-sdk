# retroSoC 固件构建脚本
# 用于编译基于RISC-V架构的嵌入式固件

# RISC-V 交叉编译工具链前缀
CROSS=riscv64-unknown-elf-
BUILD_DIR := build
PROJECT_PATH := $(shell pwd)
# 包含配置文件
-include configs/.config

# 编译器标志配置
# -mabi=ilp32: 使用ILP32 ABI
# -march=rv32im: RV32I基础指令集 + M乘除法扩展
# -ffreestanding: 独立环境编译
# -nostdlib: 不链接标准库
CFLAGS := -mabi=ilp32 \
	  -march=rv32im \
	  -Wl,-Bstatic,-T,$(BUILD_DIR)/retrosoc_sections.lds,--strip-debug \
	  -Wl,-Map=$(BUILD_DIR)/$(FIRMWARE_NAME).map,--cref \
	  -ffreestanding \
	  -nostdlib

# 根据配置文件设置链接选项
ifdef CONFIG_LINK_RAM_REGION_SRAM
LDFLAGS += -DCONFIG_LINK_RAM_REGION_SRAM
CFLAGS += -DCONFIG_LINK_RAM_REGION_SRAM
endif

ifdef CONFIG_LINK_RAM_REGION_PSRAM
LDFLAGS += -DCONFIG_LINK_RAM_REGION_PSRAM
CFLAGS += -DCONFIG_LINK_RAM_REGION_PSRAM
endif

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
SRC_PATH := ./start.S ./main.c 
SRC_PATH += $(shell find $(ECOS_SDK_HOME)/components -name "*.c")
SRC_PATH += $(shell find $(ECOS_SDK_HOME)/devices -name "*.c")

CFLAGS += -I./configs 
CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/components -type d))
CFLAGS += $(addprefix -I,$(shell find $(ECOS_SDK_HOME)/devices -type d))
		
# 链接脚本路径
LDS_PATH := ./sections.lds

# 包含内存报告工具
-include $(ECOS_SDK_HOME)/tools/scripts/mem_report.mk

# 主要构建目标：生成固件的所有格式文件
$(FIRMWARE_NAME):
	@echo "Building $(FIRMWARE_NAME)..."
	@mkdir -p $(BUILD_DIR)
	@$(CROSS)cpp -P $(LDFLAGS) -o $(BUILD_DIR)/retrosoc_sections.lds $(LDS_PATH)
	@$(CROSS)gcc $(CFLAGS) -I./ -o $(BUILD_DIR)/$@ $(SRC_PATH)
	@echo "Linking $(FIRMWARE_NAME)..."
	@$(CROSS)objcopy -O verilog $(BUILD_DIR)/$@ $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	@echo "Post-processing $(FIRMWARE_NAME).hex..."
	@sed -i 's/@30000000/@00000000/g' $(BUILD_DIR)/$(FIRMWARE_NAME).hex
	@echo "Generating $(FIRMWARE_NAME).bin..."
	@$(CROSS)objcopy -O binary  $(BUILD_DIR)/$@ $(BUILD_DIR)/$(FIRMWARE_NAME).bin
	@echo "Generating $(FIRMWARE_NAME).txt..."
	@$(CROSS)objdump -d $(BUILD_DIR)/$@ > $(BUILD_DIR)/$(FIRMWARE_NAME).txt
	$(call show_mem_usage, $(BUILD_DIR)/$@)
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
