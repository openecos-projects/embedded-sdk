.section entry, "ax"
.global _start
.type _start, @function

_start:
    la sp, _loader_stack_pointer
    call main
1:
    j 1b
