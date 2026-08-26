include $(AM_HOME)/scripts/isa/riscv.mk
include $(AM_PLATFORM_MK)
COMMON_CFLAGS += -march=$(AM_CORE_MARCH) -mabi=$(AM_CORE_MABI)
LDFLAGS       += -melf32lriscv

AM_SRCS += riscv/libgcc/div.S \
           riscv/libgcc/muldi3.S \
           riscv/libgcc/multi3.c \
           riscv/libgcc/ashldi3.c \
           riscv/libgcc/unused.c
