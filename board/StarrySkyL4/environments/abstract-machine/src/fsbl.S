.section entry, "ax"
.globl _start
.type _start, @function

/* 初始化 L4 启动栈并进入 Flash 中的一级 loader。 */
_start:
  mv s0, zero
  la sp, _stack_pointer
  call _first_bootloader

/* loader 不应返回，异常返回时保持稳定循环。 */
1:
  j 1b
