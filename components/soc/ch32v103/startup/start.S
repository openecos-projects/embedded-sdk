/* Minimal polling-only reset path. Interrupt support is intentionally absent.
 * Written for ECOS; memory/CSR conventions checked against openwch/ch32v103 EVT.
 */
.section .text.start, "ax", @progbits
.balign 4
.globl _start
.type _start, @function
.option push
.option norvc
_start:
    j .Lreset
.option pop

.Lreset:
    /* Do not enable WCH fast-interrupt mode or globally enable interrupts. */
    csrw mie, zero
    csrci mstatus, 8
    la sp, _stack_pointer
    la t0, .Ltrap
    csrw mtvec, t0

    la t0, _data_load
    la t1, _data_start
    la t2, _data_end
.Lcopy:
    bgeu t1, t2, .Lbss_start
    lbu t3, 0(t0)
    sb t3, 0(t1)
    addi t0, t0, 1
    addi t1, t1, 1
    j .Lcopy
.Lbss_start:
    la t1, _bss_start
    la t2, _bss_end
.Lclear:
    bgeu t1, t2, .Lmain
    sb zero, 0(t1)
    addi t1, t1, 1
    j .Lclear
.Lmain:
    fence rw, rw
    fence.i
    call main
.balign 4
.Ltrap:
    j .Ltrap
.size _start, . - _start
