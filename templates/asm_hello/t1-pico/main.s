/* 定义 StartySky T1-Pico 系统串口寄存器和位标志。 */
.equ UART_BASE,              0x10010000
.equ UART_THR_OFFSET,        0x00
.equ UART_DLL_OFFSET,        0x00
.equ UART_DLH_OFFSET,        0x04
.equ UART_LCR_OFFSET,        0x0C
.equ UART_MCR_OFFSET,        0x10
.equ UART_LSR_OFFSET,        0x14
.equ UART_LCR_DLAB,          0x80
.equ UART_LSR_TX_READY,      0x20
.equ UART_DIVISOR_115200,    11

/* 定义 XFlash 复位入口。 */
.section .text.start, "ax"
.globl _start
.type _start, @function

/* 初始化系统串口并输出问候信息。 */
_start:
    /* 设置系统串口寄存器基址。 */
    li t0, UART_BASE

    /* 打开分频锁存器并配置 115200 波特率。 */
    li t1, UART_LCR_DLAB
    sb t1, UART_LCR_OFFSET(t0)
    li t1, UART_DIVISOR_115200
    sb t1, UART_DLL_OFFSET(t0)
    sb zero, UART_DLH_OFFSET(t0)

    /* 配置八位数据、一个停止位和无校验格式。 */
    li t1, 0x03
    sb t1, UART_LCR_OFFSET(t0)
    sb zero, UART_MCR_OFFSET(t0)

    /* 通过轮询系统串口输出汇编模板问候信息。 */
    la a0, hello_message
    call uart_puts

    /* 输出完成后保持处理器运行。 */
.Lhalt:
    j .Lhalt

.size _start, . - _start


.section .text, "ax"
.type uart_puts, @function

/* 逐字符输出以空字符结尾的字符串。 */
uart_puts:
    /* 读取当前字符并在遇到结束标志时返回。 */
    lbu a1, 0(a0)
    beqz a1, .Lputs_done

    /* 轮询发送保持寄存器的空闲标志。 */
.Lwait_tx:
    lbu t1, UART_LSR_OFFSET(t0)
    andi t1, t1, UART_LSR_TX_READY
    beqz t1, .Lwait_tx

    /* 发送当前字符并继续处理下一字符。 */
    sb a1, UART_THR_OFFSET(t0)
    addi a0, a0, 1
    j uart_puts

    /* 恢复调用者的执行流程。 */
.Lputs_done:
    ret

.size uart_puts, . - uart_puts


/* 将串口问候信息保存在 XFlash 只读数据段。 */
.section .rodata, "a"
hello_message:
    .asciz "StartySky T1-Pico XFlash assembly hello world.\r\n"
