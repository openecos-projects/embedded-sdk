# AbstractMachine 实例化工程的公共入口。
PROJECT_ROOT := $(abspath $(CURDIR))
BUILD_DIR := $(PROJECT_ROOT)/build
CONFIG_FILE := $(PROJECT_ROOT)/configs/abstract-machine.mk

ifeq ($(wildcard $(CONFIG_FILE)),)
$(error 错误：缺少 configs/abstract-machine.mk，请重新执行 ecos init_project abstract-machine)
endif

include $(CONFIG_FILE)

ifneq ($(BOARD),starrysky-l4)
$(error 错误：AbstractMachine 环境当前只支持 starrysky-l4，配置中为 $(BOARD))
endif
ifeq ($(wildcard $(AM_HOME)/am/include/am.h),)
$(error 错误：AM_HOME 无效：$(AM_HOME))
endif
override AM_KERNELS_DIR := $(ECOS_SDK_HOME)/third_party/am-kernels
AM_KERNELS_VALID := $(shell test -d "$(AM_KERNELS_DIR)/benchmarks" \
	-a -d "$(AM_KERNELS_DIR)/kernels" \
	-a -d "$(AM_KERNELS_DIR)/tests" \
	-a -f "$(AM_KERNELS_DIR)/kernels/hello/Makefile" && echo yes)
ifneq ($(AM_KERNELS_VALID),yes)
$(error 错误：SDK 内置 am-kernels 不完整：$(AM_KERNELS_DIR))
endif

AM_BOARD_PORT_DIR := $(ECOS_SDK_HOME)/board/StarrySkyL4/environments/abstract-machine
include $(AM_BOARD_PORT_DIR)/board.mk
REQUESTED_CORE_PROFILE := $(CORE_PROFILE)
CORE_PROFILE_FILE := $(AM_BOARD_PORT_DIR)/cores/$(CORE_PROFILE).conf

ifeq ($(wildcard $(CORE_PROFILE_FILE)),)
$(error 错误：核心 profile 不存在：$(CORE_PROFILE))
endif

include $(CORE_PROFILE_FILE)
ifneq ($(CORE_PROFILE),$(REQUESTED_CORE_PROFILE))
$(error 错误：核心 profile 文件名与 CORE_PROFILE 内容不一致)
endif
ifeq ($(strip $(MARCH)),)
$(error 错误：核心 profile 缺少 MARCH)
endif
ifeq ($(strip $(MABI)),)
$(error 错误：核心 profile 缺少 MABI)
endif
include $(ECOS_SDK_HOME)/environments/abstract-machine/programs/manifest.mk
include $(ECOS_SDK_HOME)/environments/abstract-machine/programs/StarrySkyL4.mk
include $(ECOS_SDK_HOME)/environments/abstract-machine/build/program-check.mk

ifeq ($(strip $(APP)),)
.DEFAULT_GOAL := help
else
.DEFAULT_GOAL := build
include $(ECOS_SDK_HOME)/environments/abstract-machine/build/stage.mk
endif

# 显示当前工程、能力和可构建程序。
.PHONY: help
help:
	@echo "板卡：$(BOARD_ID)"
	@echo "核心 profile：$(CORE_PROFILE) ($(MARCH)/$(MABI)，状态 $(STATUS))"
	@echo "核心能力：$(strip $(AVAILABLE_CORE_FEATURES))"
	@echo "设备能力：$(strip $(AVAILABLE_DEVICE_FEATURES))"
	@echo ""
	@$(MAKE) --no-print-directory list
	@echo ""
	@echo "使用 make APP=kernels/hello 编译；microbench 首次验证推荐 MAINARGS=test。"
	@echo "使用 make list-all 查看 conditional/unsupported 程序的拒绝原因。"

# 列出当前 profile 真正满足能力要求的程序。
.PHONY: list
list:
	@echo "当前可编译程序："
	@$(foreach program,$(AM_L4_PROGRAMS),$(if $(call program_available,$(program)),printf '  %-28s [%s/%s]\n' '$(program)' '$(PROGRAM_$(program)_STATUS)' '$(PROGRAM_$(program)_VERIFY)';,)) true

# 列出全部授权状态及原因。
.PHONY: list-all
list-all:
	@echo "程序清单："
	@$(foreach program,$(AM_L4_PROGRAMS),printf '  %-28s %-12s %s\n' '$(program)' '$(PROGRAM_$(program)_STATUS)' '$(PROGRAM_$(program)_REASON)';) true

# 列出 SDK 中已登记的核心 profile。
.PHONY: cores
cores:
	@echo "已登记核心 profile："
	@for profile in "$(AM_BOARD_PORT_DIR)"/cores/*.conf; do \
		name=$$(basename "$$profile" .conf); \
		march=$$(sed -n 's/^MARCH=//p' "$$profile"); \
		status=$$(sed -n 's/^STATUS=//p' "$$profile"); \
		printf '  %-24s %-18s %s\n' "$$name" "$$march" "$$status"; \
	done

# 暂存并调用 L4 的板级 AM 构建入口。
.PHONY: build
build: stage
	@$(MAKE) --no-print-directory -f "$(AM_BOARD_PORT_DIR)/build.mk" \
		ECOS_SDK_HOME="$(ECOS_SDK_HOME)" \
		AM_HOME="$(AM_HOME)" \
		AM_STAGE_DIR="$(STAGE_DIR)" \
		AM_BUILD_ROOT="$(WORK_DIR)" \
		AM_IMAGE_DIR="$(IMAGE_DIR)" \
		AM_GENERATED_DIR="$(BUILD_DIR)" \
		AM_BOARD_ID="$(BOARD_ID)" \
		AM_PROGRAM_ID="$(APP)" \
		AM_APP_PATH="$(PROGRAM_PATH)" \
		AM_PROGRAM_AUTHORIZED=1 \
		AM_CORE_MARCH="$(MARCH)" \
		AM_CORE_MABI="$(MABI)" \
		AM_CPU_FREQ_MHZ="$(CPU_FREQ_MHZ)" \
		AM_TIMER_FREQ_MHZ="$(TIMER_FREQ_MHZ)" \
		AM_MAINARGS="$(AM_MAINARGS)"
	@echo "产物目录：$(IMAGE_DIR)"

# 只删除当前实例化工程自己的 build 目录。
.PHONY: clean
clean:
	@test "$(BUILD_DIR)" = "$(PROJECT_ROOT)/build"
	@rm -rf "$(BUILD_DIR)"

# 为无歧义的程序 basename 提供快捷目标。
define am_shortcut
.PHONY: $(notdir $(1))
$(notdir $(1)):
	@$$(MAKE) --no-print-directory APP=$(1) build
endef

$(foreach program,$(AM_L4_PROGRAMS),$(eval $(call am_shortcut,$(program))))
