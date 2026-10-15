.section .text

start:

# zero-initialize register file
addi x1, zero, 0
la sp, _stack_point
addi x3, zero, 0
addi x4, zero, 0
addi x5, zero, 0
addi x6, zero, 0
addi x7, zero, 0
addi x8, zero, 0
addi x9, zero, 0
addi x10, zero, 0
addi x11, zero, 0
addi x12, zero, 0
addi x13, zero, 0
addi x14, zero, 0
addi x15, zero, 0
addi x16, zero, 0
addi x17, zero, 0
addi x18, zero, 0
addi x19, zero, 0
addi x20, zero, 0
addi x21, zero, 0
addi x22, zero, 0
addi x23, zero, 0
addi x24, zero, 0
addi x25, zero, 0
addi x26, zero, 0
addi x27, zero, 0
addi x28, zero, 0
addi x29, zero, 0
addi x30, zero, 0
addi x31, zero, 0

#ifdef CONFIG_LINK_RAM_REGION_PSRAM
# init qspi
lui	a5,0x20007
li	a4,16
sw	a4,0(a5)
sw	zero,0(a5)
sw	zero,36(a5)
sw	zero,20(a5)
li	a4,1
sw	a4,4(a5)

# init gpio
lui a5, 0x10000
sw  zero,4(a5)
sw  zero,0(a5)

# init PSRAM
lui a5, 0x10004
li  a4, 18
sw  a4, 0(a5)
li  a4, 4
sw  a4, 4(a5)

lui a5, 0x20007
lui a4, 0x80
sw  a4, 16(a5)
lui a4, 0x66000
sw  a4, 24(a5)
li  a4, 0x102
sw  a4, 0(a5)
li  a3, 1
LOOP1:
lw  a4, 0(a5)
bne a4,a3,LOOP1

lui a4, 0x80
sw  a4, 16(a5)
lui a4, 0x99000
sw  a4, 24(a5)
li  a4, 0x102
sw  a4, 0(a5)
li  a3, 1
LOOP2:
lw  a4, 0(a5)
bne a4,a3,LOOP2

lui a4, 0x80
sw  a4, 16(a5)
lui a4, 0x35000
sw  a4, 24(a5)
li  a4, 0x102
sw  a4, 0(a5)
li  a3, 1
LOOP3:
lw  a4, 0(a5)
bne a4,a3,LOOP3

lui a5, 0x10000
lui  a4, 0x8
sw  a4, 0(a5)

lui a5, 0x10004
li  a4, 8
sw  a4, 0(a5)
li  a4, 0
sw  a4, 4(a5)
#endif

# copy data section
la a0, _sidata
la a1, _sdata
la a2, _edata
bge a1, a2, end_init_data
loop_init_data:
lw a3, 0(a0)
sw a3, 0(a1)
addi a0, a0, 4
addi a1, a1, 4
blt a1, a2, loop_init_data
end_init_data:

# zero-init bss section
la a0, _sbss
la a1, _ebss
bge a0, a1, end_init_bss
loop_init_bss:
sw zero, 0(a0)
addi a0, a0, 4
blt a0, a1, loop_init_bss
end_init_bss:

# call main
call main
loop:
j loop

