ENTRY(_start)

MEMORY
{
    flash (rx)  : ORIGIN = 0x20000000, LENGTH = 16M
    isram (rwx) : ORIGIN = 0x01000000, LENGTH = 16K
    dsram (rwx) : ORIGIN = 0x01800000, LENGTH = 16K
}

SECTIONS
{
    .text : ALIGN(4)
    {
        KEEP(*(.text.start))
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        . = ALIGN(4);
    } > flash

    /* 热点代码段：运行地址在 ISRAM 底部，加载地址在 Flash，
     * 由 start.S 启动时拷贝。ISRAM 顶部保留为向下生长的栈。 */
    _isram_text_load = LOADADDR(.isram.text);
    .isram.text : ALIGN(4)
    {
        _isram_text_start = .;
        *(.isram.text .isram.text.*)
        . = ALIGN(4);
        _isram_text_end = .;
    } > isram AT> flash

    _data_load = LOADADDR(.data);
    .data : ALIGN(4)
    {
        _data_start = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
        _data_end = .;
    } > dsram AT> flash

    .bss (NOLOAD) : ALIGN(4)
    {
        _bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > dsram

    _heap_start = ALIGN(16);
    _heap_end = ORIGIN(dsram) + LENGTH(dsram);
    /* ISRAM 与 DSRAM 统一作为 32 KiB SRAM 池：DSRAM 承载 data/bss/heap，
     * ISRAM 底部承载 .isram.text 热点代码（可为空），顶部用作栈。
     * 两个 bank 地址不连续，无法合并为单一区域。 */
    _stack_top = ORIGIN(isram) + LENGTH(isram);
    _stack_bottom = _isram_text_end;
    _stack_pointer = _stack_top;
    _flash_image_start = ORIGIN(flash);
    _flash_image_end = MAX(LOADADDR(.data) + SIZEOF(.data),
                           LOADADDR(.isram.text) + SIZEOF(.isram.text));

    ASSERT(_flash_image_end <= ORIGIN(flash) + LENGTH(flash),
           "Flash region overflow")
    ASSERT(_heap_start <= _heap_end,
           "CL1-2512 .data/.bss overflow the 16 KiB DSRAM bank")
    ASSERT(_isram_text_end <= ORIGIN(isram) + LENGTH(isram) - 0x1000,
           "CL1-2512 .isram.text overflow: keep at least 4 KiB for the stack")
}
