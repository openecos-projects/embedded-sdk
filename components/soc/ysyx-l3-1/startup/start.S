.section .text.startup, "ax"
.globl _start
.type _start, @function

_start:
    la sp, _stack_pointer
    la a0, _sidata
    la a1, _sdata
    la a2, _edata
1:
    bgeu a1, a2, 2f
    ld t0, 0(a0)
    sd t0, 0(a1)
    addi a0, a0, 8
    addi a1, a1, 8
    j 1b
2:
    la a0, _sbss
    la a1, _ebss
3:
    bgeu a0, a1, 4f
    sd zero, 0(a0)
    addi a0, a0, 8
    j 3b
4:
    call main
5:
    j 5b
