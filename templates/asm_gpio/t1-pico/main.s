/* 定义 StartySky T1-Pico GPIOA 寄存器和演示参数。 */
.equ GPIO_BASE,              0x10060000
.equ GPIO_PORTA_DR_OFFSET,   0x00
.equ GPIO_PORTA_DDR_OFFSET,  0x04
.equ GPIO_PORTA_MASK,        0xFF
.equ GPIO_DELAY_LOOPS,       2000000

/* 定义 XFlash 复位入口。 */
.section .text.start, "ax"
.globl _start
.type _start, @function

/* 初始化 GPIOA 低八位并使用软件延时周期翻转。 */
_start:
    /* 设置 GPIO 寄存器基址并预置低电平。 */
    li t0, GPIO_BASE
    sw zero, GPIO_PORTA_DR_OFFSET(t0)

    /* 将 GPIOA 低八位配置为输出并等待写入生效。 */
    li t1, GPIO_PORTA_MASK
    sw t1, GPIO_PORTA_DDR_OFFSET(t0)
    fence iorw, iorw

    /* 保存当前 GPIOA 低八位的输出状态。 */
    li t2, 0

    /* 使用软件循环产生可观察的延时。 */
.Ltoggle_loop:
    li t3, GPIO_DELAY_LOOPS
.Ldelay_loop:
    addi t3, t3, -1
    bnez t3, .Ldelay_loop

    /* 翻转 GPIOA 低八位并等待寄存器写入生效。 */
    xori t2, t2, GPIO_PORTA_MASK
    sw t2, GPIO_PORTA_DR_OFFSET(t0)
    fence iorw, iorw
    j .Ltoggle_loop

.size _start, . - _start
