.section .text.start, "ax"
.globl _start
.type _start, @function

_start:
    mv s0, zero
    la sp, _stack_pointer

    la t0, _data_load
    la t1, _data_start
    la t2, _data_end
.Lcopy_data:
    bgeu t1, t2, .Lclear_bss_start
    lbu t3, 0(t0)
    sb t3, 0(t1)
    addi t0, t0, 1
    addi t1, t1, 1
    j .Lcopy_data

.Lclear_bss_start:
    la t1, _bss_start
    la t2, _bss_end
.Lclear_bss:
    bgeu t1, t2, .Lc_runtime_ready
    sb zero, 0(t1)
    addi t1, t1, 1
    j .Lclear_bss

.Lc_runtime_ready:
    fence rw, rw
    call main

.Lhalt:
    ebreak
    j .Lhalt

.size _start, . - _start
