.section .text.trap, "ax"
.align 2
.globl startysky_t1_pico_machine_trap_entry
.type startysky_t1_pico_machine_trap_entry, @function

/* 保存完整处理器现场并调用 StartySky T1-Pico 机器模式陷阱分发函数。 */
startysky_t1_pico_machine_trap_entry:
    /* 分配十六字节对齐的陷阱现场保存空间。 */
    addi sp, sp, -128

    /* 保存除零寄存器和栈指针之外的全部通用寄存器。 */
    sw ra, 0(sp)
    sw gp, 4(sp)
    sw tp, 8(sp)
    sw t0, 12(sp)
    sw t1, 16(sp)
    sw t2, 20(sp)
    sw s0, 24(sp)
    sw s1, 28(sp)
    sw a0, 32(sp)
    sw a1, 36(sp)
    sw a2, 40(sp)
    sw a3, 44(sp)
    sw a4, 48(sp)
    sw a5, 52(sp)
    sw a6, 56(sp)
    sw a7, 60(sp)
    sw s2, 64(sp)
    sw s3, 68(sp)
    sw s4, 72(sp)
    sw s5, 76(sp)
    sw s6, 80(sp)
    sw s7, 84(sp)
    sw s8, 88(sp)
    sw s9, 92(sp)
    sw s10, 96(sp)
    sw s11, 100(sp)
    sw t3, 104(sp)
    sw t4, 108(sp)
    sw t5, 112(sp)
    sw t6, 116(sp)

    /* 保存陷阱返回地址和进入陷阱后的机器状态。 */
    csrr t0, mepc
    sw t0, 120(sp)
    csrr t0, mstatus
    sw t0, 124(sp)

    /* 将 mcause 和 mepc 传递给 C 语言陷阱分发函数。 */
    csrr a0, mcause
    csrr a1, mepc
    call startysky_t1_pico_machine_trap_handler

    /* 恢复机器状态和陷阱返回地址。 */
    lw t0, 124(sp)
    csrw mstatus, t0
    lw t0, 120(sp)
    csrw mepc, t0

    /* 恢复全部通用寄存器。 */
    lw ra, 0(sp)
    lw gp, 4(sp)
    lw tp, 8(sp)
    lw t0, 12(sp)
    lw t1, 16(sp)
    lw t2, 20(sp)
    lw s0, 24(sp)
    lw s1, 28(sp)
    lw a0, 32(sp)
    lw a1, 36(sp)
    lw a2, 40(sp)
    lw a3, 44(sp)
    lw a4, 48(sp)
    lw a5, 52(sp)
    lw a6, 56(sp)
    lw a7, 60(sp)
    lw s2, 64(sp)
    lw s3, 68(sp)
    lw s4, 72(sp)
    lw s5, 76(sp)
    lw s6, 80(sp)
    lw s7, 84(sp)
    lw s8, 88(sp)
    lw s9, 92(sp)
    lw s10, 96(sp)
    lw s11, 100(sp)
    lw t3, 104(sp)
    lw t4, 108(sp)
    lw t5, 112(sp)
    lw t6, 116(sp)

    /* 释放陷阱现场并返回被中断程序。 */
    addi sp, sp, 128
    mret

.size startysky_t1_pico_machine_trap_entry, . - startysky_t1_pico_machine_trap_entry
