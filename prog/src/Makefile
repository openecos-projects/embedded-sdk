MAKEFLAGS += --no-print-directory
# 默认参数
APP_NAME ?= ysyx_hello
APP_TYPE ?= mem
APP_ARCH = riscv64-mycpu

# 生成文件名
APP_ORG_BIN = $(APP_NAME)-$(APP_ARCH).bin
APP_ORG_ELF = $(APP_NAME)-$(APP_ARCH).elf
APP_LOD_BIN = loader-$(APP_ARCH).bin
APP_LOD_ELF = loader-$(APP_ARCH).elf
APP_STD_BIN = $(APP_NAME)-$(APP_TYPE).bin
APP_STD_ELF = $(APP_NAME)-$(APP_TYPE).elf

# 目录定义
HOME_DIR = $(shell pwd)
BIN_DIR = $(HOME_DIR)/../bin/$(APP_TYPE)

# 验证APP_TYPE是否有效
VALID_TYPES := flash mem psram sram flashsram
ifneq ($(filter $(APP_TYPE),$(VALID_TYPES)),$(APP_TYPE))
$(error Invalid APP_TYPE: $(APP_TYPE). Must be one of: $(VALID_TYPES))
endif

# 定义地址常量
FLASH_ADDR = 0x30000000
MEM_ADDR = 0x80000000
PSRAM_ADDR = 0xA0000000
SRAM_ADDR = 0x0F000000


# 帮助信息
help:
	@echo "RISC-V Application Builder"
	@echo "Usage: make [TARGET] [APP_NAME=name] [APP_TYPE=type]"
	@echo "Valid types: flash, mem, psram, sram (default: mem)"
	@echo "Default app name: ysyx_hello"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the application (default)"
	@echo "  clean      - Clean build artifacts"
	@echo "  help       - Show this help message"

# 默认目标
all: build
	
# 构建目标
build:
	@echo "Building $(APP_NAME) for $(APP_TYPE)..."
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) build-$(APP_TYPE)

# 修改链接脚本
chg_ld_script:
#	@echo "Changing linker script to core_$(APP_TYPE)..."
	@sed -i 's/core_[a-z]\+/core_$(APP_TYPE)/' $$AM_HOME/scripts/$(APP_ARCH).mk

# 修改加载地址
chg_ld_addr:
#	@echo "Setting load address to $(ADDR)..."
	@sed -i 's/\(pmem_start=\)0x[0-9A-Z]\+/\1$(ADDR)/' $$AM_HOME/scripts/$(APP_ARCH).mk

# 修改栈指针
chg_ld_sp:
#	@echo "Setting stack pointer to $(ADDR)..."
	@sed -i 's/\(_stack_top = ALIGN\)(0x[0-9A-Z]\+)/\1($(ADDR))/' $$AM_HOME/scripts/platform/core_flash.ld

# 复制操作
copy_oper:
#	@echo "Copying output files..."
	mkdir -p build
ifneq ($(APP_TYPE),flash)
	mv $(HOME_DIR)/loader/build/$(APP_LOD_BIN) build/$(APP_STD_BIN)
	mv $(HOME_DIR)/loader/build/$(APP_LOD_ELF) build/$(APP_STD_ELF)
else
	mv build/$(APP_ORG_BIN) build/$(APP_STD_BIN)
	mv build/$(APP_ORG_ELF) build/$(APP_STD_ELF)
endif
	mkdir -p $(BIN_DIR)
	cp build/$(APP_STD_BIN) build/$(APP_STD_ELF) $(BIN_DIR)/
	cd $(BIN_DIR) && \
	riscv64-linux-gnu-objdump -d $(APP_STD_ELF) > prog.txt && \
	riscv64-linux-gnu-objcopy -I binary -O verilog $(APP_STD_BIN) prog.flash

# Flash类型构建
build-flash:
	$(MAKE) chg_ld_script APP_TYPE=flash
	$(MAKE) chg_ld_addr ADDR=$(FLASH_ADDR)
	cd $(APP_NAME) && \
	if [ "$(APP_NAME)" != "rtthread" ]; then \
		make ARCH=$(APP_ARCH); \
	else \
		cp *.c rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.cc rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.h rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cd rt-thread/bsp/qemu-riscv-virt64/; \
		sed -i 's/^FLASH = 0/FLASH = 1/' rtconfig.py; \
		scons; \
		cp rtthread.bin $(APP_STD_BIN); \
		cp rtthread.elf $(APP_STD_ELF); \
		mv $(APP_STD_BIN) $(APP_STD_ELF) $(BIN_DIR)/; \
	fi
ifneq ($(APP_NAME),rtthread)
	$(MAKE) copy_oper
endif

# Mem类型构建
build-mem:
	$(MAKE) -s chg_ld_script APP_TYPE=mem
	$(MAKE) -s chg_ld_addr ADDR=$(MEM_ADDR)
	cd $(APP_NAME) && \
	if [ "$(APP_NAME)" != "rtthread" ]; then \
		make ARCH=$(APP_ARCH) LOAD_TYPE=-DMEM_LOAD; \
		cp build/$(APP_ORG_BIN) $(HOME_DIR)/loader; \
	else \
		cp *.c rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.cc rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.h rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cd rt-thread/bsp/qemu-riscv-virt64/; \
		sed -i 's/^FLASH = 1/FLASH = 0/' rtconfig.py; \
		scons; \
		cp rtthread.bin $(HOME_DIR)/loader/$(APP_ORG_BIN); \
	fi
	$(MAKE) -s chg_ld_script APP_TYPE=flash
	$(MAKE) -s chg_ld_addr ADDR=$(FLASH_ADDR)
	cd $(HOME_DIR)/loader && \
	sed -i 's/^\(BIN_PATH\s\+=\s\+\)\(.\+\)/\1$(APP_NAME)-$(APP_ARCH)\.bin/' Makefile && \
	make ARCH=$(APP_ARCH)
	$(MAKE) copy_oper

# PSRAM类型构建
build-psram:
	$(MAKE) chg_ld_script APP_TYPE=psram
	$(MAKE) chg_ld_addr ADDR=$(PSRAM_ADDR)
	cd $(APP_NAME) && \
	if [ "$(APP_NAME)" != "rtthread" ]; then \
		make ARCH=$(APP_ARCH) LOAD_TYPE=-DMEM_LOAD; \
		cp build/$(APP_ORG_BIN) $(HOME_DIR)/loader; \
	else \
		cp *.c rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.cc rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.h rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cd rt-thread/bsp/qemu-riscv-virt64/; \
		sed -i 's/^FLASH = 1/FLASH = 0/' rtconfig.py; \
		scons; \
		cp rtthread.bin $(HOME_DIR)/loader/$(APP_ORG_BIN); \
	fi
	$(MAKE) chg_ld_script APP_TYPE=flash
	$(MAKE) chg_ld_addr ADDR=$(FLASH_ADDR)
	cd $(HOME_DIR)/loader && \
	sed -i 's/^\(BIN_PATH\s\+=\s\+\)\(.\+\)/\1$(APP_NAME)-$(APP_ARCH)\.bin/' Makefile && \
	make ARCH=$(APP_ARCH)
	$(MAKE) copy_oper

# SRAM类型构建
build-sram:
#	@echo "sram type"
	@$(MAKE) chg_ld_script APP_TYPE=sram
	@$(MAKE) -s chg_ld_addr ADDR=$(SRAM_ADDR)

	@cd $(APP_NAME) && \
	if [ "$(APP_NAME)" != "rtthread" ]; then \
		make  ARCH=$(APP_ARCH) LOAD_TYPE=-DMEM_LOAD; \
		cp build/$(APP_ORG_BIN) $(HOME_DIR)/loader; \
	else \
		cp *.c rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.cc rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cp *.h rt-thread/bsp/qemu-riscv-virt64/applications/; \
		cd rt-thread/bsp/qemu-riscv-virt64/; \
		sed -i 's/^FLASH = 1/FLASH = 0/' rtconfig.py; \
		scons; \
		cp rtthread.bin $(HOME_DIR)/loader/$(APP_ORG_BIN); \
	fi

	@$(MAKE) -s chg_ld_script APP_TYPE=flashsram
	@$(MAKE) -s chg_ld_addr ADDR=$(FLASH_ADDR)

	@cd $(HOME_DIR)/loader && \
	sed -i 's/^\(BIN_PATH\s\+=\s\+\)\(.\+\)/\1$(APP_NAME)-$(APP_ARCH)\.bin/' Makefile && \
	make -s ARCH=$(APP_ARCH)

	@$(MAKE) -s copy_oper

# 清理目标
clean:
	@echo "Cleaning build artifacts..."
	rm -rf ./*/build
	rm -f ./build/*
	rm -f ./loader/*.bin

clean-bin:
	@echo "Cleaning binary files..."
	rm -rf ../bin/mem/*
	rm -rf ../bin/flash/*
	rm -rf ../bin/psram/*
	rm -rf ../bin/sram/*
.PHONY: all help build build-flash build-mem build-psram build-sram \
        chg_ld_script chg_ld_addr chg_ld_sp copy_oper clean

