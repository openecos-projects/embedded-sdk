.section .text.startup, "ax"
.globl _start
.type _start, @function

_start:
#ifdef CONFIG_LINK_RAM_REGION_PSRAM
    lui sp, 0x30020

    /* Match the legacy C2 PSRAM controller and QSPI bring-up before using PSRAM. */
    lui a5, 0x20007
    li a4, 16
    sw a4, 0(a5)
    sw zero, 0(a5)
    sw zero, 36(a5)
    sw zero, 20(a5)
    li a4, 1
    sw a4, 4(a5)

    lui a5, 0x10000
    sw zero, 4(a5)
    sw zero, 0(a5)

    lui a5, 0x10004
    li a4, 18
    sw a4, 0(a5)
    li a4, 4
    sw a4, 4(a5)

    lui a5, 0x20007
    lui a4, 0x80
    sw a4, 16(a5)
    lui a4, 0x66000
    sw a4, 24(a5)
    li a4, 0x102
    sw a4, 0(a5)
    li a3, 1
1:
    lw a4, 0(a5)
    bne a4, a3, 1b

    lui a4, 0x80
    sw a4, 16(a5)
    lui a4, 0x99000
    sw a4, 24(a5)
    li a4, 0x102
    sw a4, 0(a5)
2:
    lw a4, 0(a5)
    bne a4, a3, 2b

    lui a4, 0x80
    sw a4, 16(a5)
    lui a4, 0x35000
    sw a4, 24(a5)
    li a4, 0x102
    sw a4, 0(a5)
3:
    lw a4, 0(a5)
    bne a4, a3, 3b

    lui a5, 0x10000
    lui a4, 0x8
    sw a4, 0(a5)

    lui a5, 0x10004
    li a4, 8
    sw a4, 0(a5)
    sw zero, 4(a5)
#endif

    la sp, _stack_pointer
    la a0, _sidata
    la a1, _sdata
    la a2, _edata
1:
    bgeu a1, a2, 2f
    lw t0, 0(a0)
    sw t0, 0(a1)
    addi a0, a0, 4
    addi a1, a1, 4
    j 1b
2:
    la a0, _sbss
    la a1, _ebss
3:
    bgeu a0, a1, 4f
    sw zero, 0(a0)
    addi a0, a0, 4
    j 3b
4:
    call main
5:
    j 5b
