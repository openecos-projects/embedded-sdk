.section entry
.global _start
_start:
    la sp, _stack_pointer
    call main
loop:
    j loop
