CROSS_COMPILE := riscv64-linux-gnu-
COMMON_FLAGS  := -fno-pic  -march=rv64ifd_zifencei -mcmodel=medany
CFLAGS        += $(COMMON_FLAGS) $(KAZA_CFLAGS) -static
ASFLAGS       += $(COMMON_FLAGS) -O0
LDFLAGS       += -melf64lriscv

AM_SRCS := mycpu/start.S \
           mycpu/libgcc/muldi3.S \
           mycpu/libgcc/div.S \
           mycpu/trm.c \
           mycpu/ioe.c \
           mycpu/timer.c \
           mycpu/input.c \
           mycpu/cte.c \
           mycpu/trap.S \
           mycpu/vme.c \
           mycpu/mpe.c \
           mycpu/uart.c

CFLAGS    += -fdata-sections -ffunction-sections
LDFLAGS   += -T $(AM_HOME)/scripts/platform/core_flash.ld --defsym=_pmem_start=0x30000000 --defsym=_entry_offset=0x0
LDFLAGS   += --gc-sections -e _start
CFLAGS += -DMAINARGS=\"$(mainargs)\"
.PHONY: $(AM_HOME)/am/src/mycpu/trm.c

image: $(IMAGE).elf
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin
