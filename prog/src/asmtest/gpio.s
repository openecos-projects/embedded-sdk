.global _start
_start:
GPIO_INIT:
    lui a4,0x10002
    li a5,0xff
    sb a5,0(a4)
    li a5,0x00
    sb a5,8(a4)

END:
    j END
