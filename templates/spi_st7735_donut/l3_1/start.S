.section entry
.global _start

_start:
    /* 1. Setup Stack */
    la sp, _stack_pointer

#ifdef CONFIG_LINK_TARGET_XIP
    /* 2. Copy .data from ROM to MEM in XIP mode */
    la a0, _sidata
    la a1, _sdata
    la a2, _edata
    bgeu a1, a2, 2f
1:
    lw t0, 0(a0)
    sw t0, 0(a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b
2:
#endif

    /* 3. Clear .bss */
    la a0, _sbss
    la a1, _ebss
    bgeu a0, a1, 4f
    li t0, 0
3:
    sw t0, 0(a0)
    addi a0, a0, 4
    bltu a0, a1, 3b
4:

    /* 4. Call main */
    call main

loop:
    j loop
