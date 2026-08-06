/* 定义 StartySky T1 Flash XIP 固件的复位入口。 */
.section .text.start, "ax"
.globl _start
.type _start, @function

/* 初始化 C 语言运行环境并进入主程序。 */
_start:
    /* 清零帧指针并设置链接脚本提供的栈顶。 */
    mv s0, zero
    la sp, _stack_pointer

    /* 将初始化数据从 Flash 逐字节搬运到 DSRAM。 */
    la t0, _data_load
    la t1, _data_start
    la t2, _data_end
.Lcopy_data:
    bgeu t1, t2, .Lclear_bss_start
    lbu t3, 0(t0)
    sb t3, 0(t1)
    addi t0, t0, 1
    addi t1, t1, 1
    j .Lcopy_data

    /* 将 DSRAM 中的未初始化数据段逐字节清零。 */
.Lclear_bss_start:
    la t1, _bss_start
    la t2, _bss_end
.Lclear_bss:
    bgeu t1, t2, .Lc_runtime_ready
    sb zero, 0(t1)
    addi t1, t1, 1
    j .Lclear_bss

    /* 等待存储器访问完成后调用主函数。 */
.Lc_runtime_ready:
    fence rw, rw
    call main

    /* 主函数返回后保持处理器停止在稳定循环中。 */
.Lhalt:
    ebreak
    j .Lhalt

.size _start, . - _start
