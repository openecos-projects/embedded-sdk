/* @BSP_NAME@ 内存布局。 */
/* [必须修改] TODO_BSP_REQUIRED：根据芯片手册填写真实地址和长度。 */
ENTRY(_start)

MEMORY {
    flash (rx)  : ORIGIN = 0x00000000, LENGTH = 1K
    ram   (rwx) : ORIGIN = 0x00001000, LENGTH = 1K
}

SECTIONS {
    .text : {
        KEEP(*(.text.start))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > flash

    .data : {
        _sdata = .;
        *(.data .data.*)
        _edata = .;
    } > ram AT> flash
    _sidata = LOADADDR(.data);

    .bss (NOLOAD) : {
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        _ebss = .;
    } > ram

    /* [按需修改] 栈大小要根据中断和函数调用深度确认。 */
    _stack_pointer = ORIGIN(ram) + LENGTH(ram);
}
