.global _start
_start:
    li s0, 0
    # sp: 0xFC
    auipc sp, 0x101
    addi sp, sp, -4  #FC101000
UART_INIT:
    lui a4, 0x10000
    lbu a5, 3(a4) # 10000003 <_entry_offset+0x10000003>
    ori a5, a5, -128
    andi a5, a5, 255
    sb a5, 3(a4)
    li a5, 13
    sb a5, 0(a4)
    li a5, 3
    sb a5, 3(a4)

HELLO_INFO:
    la s0, msg_hello
    jal ra, STR_PUT_FUNC
    # j SDRAM_FAIL

MEM_TEST:
# wr&rd range [0xFC10_xxxx, 0xFC10_1000]
    mv s1, sp #0xFC10_1000
    li s4, 0
    li s5, 9
SDRAM_LOOP:
    mv s2, s4
    sd s2, 0(sp)
    ld s3, 0(sp)

    la s0, msg_addr
    jal ra, STR_PUT_FUNC
    mv a1, sp
    jal ra, NUM_PUT_FUNC
    bne s2, s3, SDRAM_FAIL
    addi s4, s4, 1
    addi sp, sp, -8
    ble s4, s5, SDRAM_LOOP # if s4 <= s5 then SDRAM_LOOP
SDRAM_PASS:
    la s0, msg_pass
    j SDRAM_RES
SDRAM_FAIL:
    mv sp, s1 # restore sp
    la s0, msg_fail
SDRAM_RES:
    jal STR_PUT_FUNC
END:
    j END

STR_PUT_FUNC:
    lbu a0, 0(s0)
    sd ra, 0(sp) # for recursive func call
STR_LOOP:
    addi s0, s0, 1
    jal ra, PUTC_FUNC
    lbu a0, 0(s0)
    bnez a0, STR_LOOP
    ld ra, 0(sp)
    ret

NUM_PUT_FUNC:
    sd ra, 0(sp)
    li t0, 10
NUM_LOOP:
    andi a0, a1, 0xf
    # handle a0(0~15)
    bge a0, t0, ALPHA_CHAR # a0 >= 10, a0 - 10 + 'A'
    addi a0, a0, 0x30 # a0 <= 9, a0 + '0'
    j NUM_RES
ALPHA_CHAR:
    sub a0, a0, t0
    addi a0, a0, 0x41
NUM_RES:
    jal ra, PUTC_FUNC
    srli a1, a1, 4
    bnez a1, NUM_LOOP
    # output '\n'
    li a0, 0x0A
    jal ra, PUTC_FUNC
    ld ra, 0(sp)
    ret

PUTC_FUNC:
    lui a4, 0x10000
WHILE:
    lbu	a5, 5(a4) # 10000005 <_start-0x1ffffffb>
    andi a5, a5, 32
    beqz a5, WHILE
    sb a0, 0(a4)
    ret

.section .data
msg_hello: .string "start asm sdram test...\n"
msg_addr: .string "[sdram test]addr: 0x"
msg_pass: .string "sdram test pass!\n"
msg_fail: .string "sdram test fail!\n"
