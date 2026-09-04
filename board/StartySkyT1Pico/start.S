/* 定义 StartySky T1-Pico 固件的 Flash 复位入口。 */
.section .text.start, "ax"
.globl _start
.type _start, @function

/* 初始化 C 语言运行环境并进入主程序。 */
_start:
    /* 清零帧指针以建立初始调用环境。 */
    mv s0, zero

#ifdef CONFIG_LINK_TARGET_SDRAM
    /* 在使用 SDRAM 栈之前通过无栈汇编初始化控制器。 */
    call sdram_init
#endif

    /* 设置链接脚本提供的栈顶。 */
    la sp, _stack_pointer

#ifdef CONFIG_LINK_TARGET_SDRAM
    /* 搬运并执行链接到 SDRAM 的二级加载器。 */
    call _first_bootloader
#else
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
#endif

    /* 主函数返回后保持处理器停止在稳定循环中。 */
.Lhalt:
    ebreak
    j .Lhalt

.size _start, . - _start


#ifdef CONFIG_LINK_TARGET_SDRAM
.globl sdram_init
.type sdram_init, @function

/* 按照十六位数据总线配置初始化 SDRAM。 */
sdram_init:
    /* 设置 SDRAM 控制器寄存器基址。 */
    lui t0, 0x100B0

    /* 发送第一次空操作命令并等待启动时序。 */
    li t1, 0xB
    sw t1, 4(t0)
    lw t2, 4(t0)

    /* 执行第一次空操作命令后的启动延时。 */
    li t3, 100
.Lsdram_delay_100:
    addi t3, t3, -1
    bnez t3, .Lsdram_delay_100

    /* 发送第二次空操作命令并等待启动时序。 */
    li t1, 0xB
    sw t1, 4(t0)
    lw t2, 4(t0)

    /* 执行第二次空操作命令后的启动延时。 */
    li t3, 200
.Lsdram_delay_200:
    addi t3, t3, -1
    bnez t3, .Lsdram_delay_200

    /* 发送所有存储体预充电命令。 */
    li t1, 0x9
    sw t1, 4(t0)
    lw t2, 4(t0)

    /* 设置初始刷新周期并等待刷新时序。 */
    li t1, 0xA
    sw t1, 8(t0)
    lw t2, 8(t0)

    /* 执行初始刷新周期延时。 */
    li t3, 9
.Lsdram_delay_9:
    addi t3, t3, -1
    bnez t3, .Lsdram_delay_9

    /* 将刷新周期设置为正常工作值。 */
    li t1, 0x320
    sw t1, 8(t0)
    lw t2, 8(t0)

    /* 发送模式寄存器设置命令。 */
    li t1, 0xA
    sw t1, 4(t0)
    lw t2, 4(t0)
    fence rw, rw

    /* 通过指定地址的读访问向 SDRAM 提供模式位。 */
    lui t1, 0x80033
    lw t4, 0(t1)
    fence rw, rw

    /* 配置十六位数据总线并使控制器进入正常工作状态。 */
    li t1, 0x01F8000A
    sw t1, 0(t0)
    lw t2, 0(t0)

    /* 清零命令寄存器以进入正常工作模式。 */
    li t1, 0x0
    sw t1, 4(t0)
    lw t2, 4(t0)
    fence rw, rw

    /* 返回 Flash 中的复位流程。 */
    ret

.size sdram_init, . - sdram_init
#endif
