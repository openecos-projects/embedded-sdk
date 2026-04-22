.global _start
_start:
UART_INIT:
    lui a4,0x10000
    lbu a5,3(a4) # 10000003 <_entry_offset+0x10000003>
    ori a5,a5,-128
    andi a5,a5,255
    # li a5, 0x83
    sb a5,3(a4)
    li a5,13
    # li a5, 81
    # li a5, 16
    sb a5,0(a4)
    li a5,3
    sb a5,3(a4)
HELLO_INIT:
    la s0, msg_hello
    li a0, 72
HELLO_LOOP:
    addi s0, s0, 1
    jal ra, PUTC
    lbu a0, 0(s0)
    bnez a0, HELLO_LOOP

APP_INIT:
    li t1, 1
    li t2, 6
APP_LOOP:
    la s0, msg_luck
    li a0, 72
INTER_LOOP:
    addi s0, s0, 1
    jal ra, PUTC
    lbu a0, 0(s0)
    bnez a0, INTER_LOOP
CHECK:
    addi t1, t1, 1
    ble t1, t2, APP_LOOP
END:
    j END

PUTC:
    lui a4,0x10000
WHILE:
    lbu	a5,5(a4) # 10000005 <_start-0x1ffffffb>
    andi a5,a5,32
    beqz a5, WHILE
    sb a0,0(a4)
    ret

.section .data
msg_hello: .string "Hello YSYX!\n"
msg_luck: .string "Have a good luck!\n"
