/* Reset path: interrupts stay disabled until the application enables them.
 * If the interrupt capability is linked, install the PFIC software-context
 * vectors after .bss initialization. Polling-only applications keep a halt trap.
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
    /* No WCH fast ABI. Clear both global/previous interrupt-enable bits. */
    csrw mie, zero
    li t0, 0x88
    csrc mstatus, t0
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
    .weak hal_interrupt_init
    la t0, hal_interrupt_init
    beqz t0, .Lapplication
    jalr ra, t0, 0
.Lapplication:
    call main
.balign 4
.Ltrap:
    j .Ltrap
.size _start, . - _start
