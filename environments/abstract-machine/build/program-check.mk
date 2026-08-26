# 根据 core profile 和板级能力计算当前可用特性。
AVAILABLE_CORE_FEATURES := rv32e \
	$(if $(filter 1,$(HAS_ZICSR)),zicsr) \
	$(if $(filter 1,$(HAS_ECALL)),ecall) \
	$(if $(filter 1,$(HAS_MRET)),mret) \
	$(if $(filter 1,$(CTE_SUPPORTED)),cte) \
	$(if $(filter 1,$(HAS_M_EXTENSION)),m) \
	$(if $(filter 1,$(HAS_A_EXTENSION)),a)
AVAILABLE_DEVICE_FEATURES := \
	$(if $(filter 1,$(HAS_UART_TX)),uart-tx) \
	$(if $(filter 1,$(HAS_UART_RX)),uart-rx) \
	$(if $(filter 1,$(HAS_TIMER)),timer) \
	$(if $(filter 1,$(HAS_RTC)),rtc) \
	$(if $(filter 1,$(HAS_FLASH)),flash) \
	$(if $(filter 1,$(HAS_PS2)),input) \
	$(if $(filter 1,$(HAS_GPU)),gpu) \
	$(if $(filter 1,$(HAS_AUDIO)),audio) \
	$(if $(filter 1,$(HAS_DISK)),disk) \
	$(if $(filter 1,$(HAS_NET)),net)

# 计算某个程序在当前 profile 下缺失的能力。
missing_core = $(filter-out $(AVAILABLE_CORE_FEATURES),$(PROGRAM_$(1)_CORE_FEATURES))
missing_device = $(filter-out $(AVAILABLE_DEVICE_FEATURES),$(PROGRAM_$(1)_DEVICE_FEATURES))
program_available = $(and $(filter supported,$(PROGRAM_$(1)_STATUS)),\
	$(if $(call missing_core,$(1)),,yes),$(if $(call missing_device,$(1)),,yes))

ifneq ($(strip $(APP)),)
ifeq ($(filter $(APP),$(AM_L4_PROGRAMS)),)
$(error 错误：未知或未授权程序 $(APP)。请执行 make list-all 查看清单)
endif

PROGRAM_PATH := $(PROGRAM_$(APP)_PATH)
PROGRAM_STATUS := $(PROGRAM_$(APP)_STATUS)
PROGRAM_CORE_FEATURES := $(PROGRAM_$(APP)_CORE_FEATURES)
PROGRAM_DEVICE_FEATURES := $(PROGRAM_$(APP)_DEVICE_FEATURES)
PROGRAM_HOST_TOOLS := $(PROGRAM_$(APP)_HOST_TOOLS)
MISSING_CORE_FEATURES := $(call missing_core,$(APP))
MISSING_DEVICE_FEATURES := $(call missing_device,$(APP))

ifneq ($(APP),$(PROGRAM_PATH))
$(error 错误：APP $(APP) 与清单路径 $(PROGRAM_PATH) 不一致)
endif
ifeq ($(PROGRAM_STATUS),unsupported)
$(error 错误：程序 $(APP) 不支持 StarrySkyL4：$(PROGRAM_$(APP)_REASON))
endif
ifneq ($(PROGRAM_STATUS),supported)
$(error 错误：程序 $(APP) 当前为 conditional：$(PROGRAM_$(APP)_REASON))
endif
ifneq ($(strip $(MISSING_CORE_FEATURES)),)
$(error 错误：程序 $(APP) 缺少核心能力 $(MISSING_CORE_FEATURES)；当前 profile 为 $(CORE_PROFILE)。$(PROGRAM_$(APP)_REASON))
endif
ifneq ($(strip $(MISSING_DEVICE_FEATURES)),)
$(error 错误：程序 $(APP) 缺少设备能力 $(MISSING_DEVICE_FEATURES)；$(PROGRAM_$(APP)_REASON))
endif
ifneq ($(strip $(PROGRAM_HOST_TOOLS)),)
$(foreach tool,$(PROGRAM_HOST_TOOLS),$(if $(shell command -v $(tool) 2>/dev/null),,$(error 错误：程序 $(APP) 需要主机工具 $(tool))))
endif

AM_MAINARGS :=
ifneq ($(strip $(MAINARGS)),)
ifneq ($(APP),benchmarks/microbench)
$(error 错误：只有 benchmarks/microbench 支持 MAINARGS)
endif
ifneq ($(words $(strip $(MAINARGS))),1)
$(error 错误：microbench MAINARGS 必须是单个规模名称)
endif
ifeq ($(filter $(strip $(MAINARGS)),test train ref huge),)
$(error 错误：microbench MAINARGS 只允许 test/train/ref/huge)
endif
AM_MAINARGS := $(strip $(MAINARGS))
endif
endif
