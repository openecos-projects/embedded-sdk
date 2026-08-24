CFLAGS    += -fdata-sections -ffunction-sections

ifneq ($(YSYX_L4),1)
$(error 错误：SDK 中的 ysyxsoc 平台只允许通过 StarrySkyL4 ysyx-am 前端构建)
endif
ifneq ($(YSYX_PROGRAM_AUTHORIZED),1)
$(error 错误：必须由 ysyx-am 工程前端授权后才能构建 L4 AM 程序)
endif
ifneq ($(YSYX_BOARD_ID),starrysky-l4)
$(error 错误：ysyx-am 当前只支持 starrysky-l4，收到 $(YSYX_BOARD_ID))
endif
ifeq ($(strip $(YSYX_PROGRAM_ID)),)
$(error 错误：缺少 YSYX_PROGRAM_ID，拒绝绕过程序清单构建)
endif
ifneq ($(YSYX_PROGRAM_ID),$(YSYX_APP_PATH))
$(error 错误：程序授权 ID $(YSYX_PROGRAM_ID) 与实际路径 $(YSYX_APP_PATH) 不一致)
endif
ifeq ($(strip $(YSYX_CORE_MARCH)),)
$(error 错误：缺少经过校验的核心 MARCH)
endif
ifeq ($(strip $(YSYX_CORE_MABI)),)
$(error 错误：缺少经过校验的核心 MABI)
endif

AM_SRCS := riscv/ysyxsoc/trm.c \
			riscv/ysyxsoc/ioe.c \
			riscv/ysyxsoc/vme.c \
			riscv/ysyxsoc/mpe.c \
			riscv/ysyxsoc/fsbl.S \
			riscv/ysyxsoc/bootloader.c
AM_EXTERNAL_SRCS += $(YSYX_AM_HAL_SRCS)
INC_PATH += $(ECOS_SDK_HOME)/hal/sys_uart \
			$(ECOS_SDK_HOME)/hal/timer \
			$(ECOS_SDK_HOME)/hal/qspi \
			$(ECOS_SDK_HOME)/board/StarrySkyL4 \
			$(YSYX_GENERATED_DIR)
LDSCRIPTS += $(ECOS_SDK_HOME)/board/StarrySkyL4/ysyx-am.lds
LDFLAGS   += --gc-sections -e _start --undefined=_trm_init
CFLAGS    += -ffreestanding -nostdlib -Werror=implicit-function-declaration
CXXFLAGS  += -fno-threadsafe-statics

YSYX_QSPI_OBJS := $(addprefix $(DST_DIR)/,$(addsuffix .o,$(basename $(filter %/qspi.c,$(AM_EXTERNAL_SRCS)))))
$(YSYX_QSPI_OBJS): CFLAGS += -O0

MAINARGS_MAX_LEN = 64
CFLAGS += -DMAINARGS_MAX_LEN=$(MAINARGS_MAX_LEN) \
	-DMAINARGS_PLACEHOLDER=CONFIG_MAINARGS

image: image-dep
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S -O binary $(IMAGE).elf $(IMAGE).bin
	@$(OBJCOPY) --change-addresses -0x30000000 -O verilog $(IMAGE).elf $(IMAGE).hex
