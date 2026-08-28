# StarrySkyL4 build bindings for AbstractMachine applications.
CROSS_COMPILE ?= riscv64-unknown-elf-
AM_ARCH := riscv32e-ysyxsoc
AM_PORT_DIR := $(AM_BOARD_PORT_DIR)
AM_PLATFORM_MK := $(AM_PORT_DIR)/platform.mk
AM_HAL_SRCS := $(ECOS_SDK_HOME)/components/soc/ysyx-2512/hal/sys_uart/sys_uart.c \
	$(ECOS_SDK_HOME)/components/soc/ysyx-2512/hal/timer/timer.c \
	$(ECOS_SDK_HOME)/components/soc/ysyx-2512/hal/qspi/qspi.c
