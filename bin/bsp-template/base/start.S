# @BSP_NAME@ 复位入口。
# [不要修改] _start 名称必须与 Makefile 和 sections.lds 的入口一致。
.section .text.start, "ax"
.globl _start
.type _start, @function

_start:
    # [必须修改] TODO_BSP_REQUIRED：确认上电状态允许直接设置栈，并补充芯片初始化。
    la sp, _stack_pointer

    # [必须修改] TODO_BSP_REQUIRED：按 sections.lds 完成 .data 搬运和 .bss 清零。

    # [不要修改] 普通 C 工程最终进入 main。
    call main

1:
    j 1b
