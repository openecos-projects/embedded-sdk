.global _start
_start:
GPIO_INIT:
    lui a4,0x10002
    
    # Configure GPIO_26 and GPIO_27 as output (PADDIR |= 0x0C000000)
    lw a5,0(a4)
    li t0,0x0c000000
    or a5,a5,t0
    sw a5,0(a4)

    # Set GPIO_26 and GPIO_27 to high level (PADOUT |= 0x0C000000)
    lw a5,8(a4)
    li t0,0x0c000000
    or a5,a5,t0
    sw a5,8(a4)

END:
    j END
