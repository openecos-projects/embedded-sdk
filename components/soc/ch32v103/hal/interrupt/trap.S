/* Original ECOS RV32 software-context entry for the CH32V103 PFIC.
 * mtvec MODE=1 uses 4-byte jump instructions. Hardware stacking and nesting
 * are disabled in hal_interrupt_init; no WCH-Interrupt-fast ABI is needed.
 */
.section .text.ch32v103_vectors, "ax", @progbits
.balign 256
.option push
.option norvc
.option norelax
.globl ecos_ch32v103_vector_table
.type ecos_ch32v103_vector_table, @function
ecos_ch32v103_vector_table:
    .rept 60
    j ecos_ch32v103_trap_entry
    .endr
.globl ecos_ch32v103_vector_table_end
ecos_ch32v103_vector_table_end:
.size ecos_ch32v103_vector_table, . - ecos_ch32v103_vector_table
.option pop

.section .text.ch32v103_trap, "ax", @progbits
.balign 4
.option push
.option norvc
.option norelax
.globl ecos_ch32v103_trap_entry
.type ecos_ch32v103_trap_entry, @function
ecos_ch32v103_trap_entry:
    /* 30 GPRs + mepc/mstatus/mcause/mtval + padding; preserve 16-byte ABI. */
    addi sp, sp, -144
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
    csrr t0, mepc
    sw t0, 120(sp)
    csrr t0, mstatus
    sw t0, 124(sp)
    csrr a0, mcause
    sw a0, 128(sp)
    lw a1, 120(sp)
    csrr a2, mtval
    sw a2, 132(sp)
    call ecos_ch32v103_trap_handler
    lw t0, 120(sp)
    csrw mepc, t0
    lw t0, 124(sp)
    csrw mstatus, t0
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
    addi sp, sp, 144
    mret
.globl ecos_ch32v103_trap_entry_end
ecos_ch32v103_trap_entry_end:
.size ecos_ch32v103_trap_entry, . - ecos_ch32v103_trap_entry
.option pop

.section .note.GNU-stack, "", @progbits
