ENTRY(_start)

MEMORY
{
    flash (rx)  : ORIGIN = 0x20000000, LENGTH = 16M
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
    _stack_top = ORIGIN(dsram) + LENGTH(dsram);
    _stack_bottom = _stack_top - 0x1000;
    _stack_pointer = _stack_top;
    _heap_end = _stack_bottom;
    _flash_image_start = ORIGIN(flash);
    _flash_image_end = LOADADDR(.data) + SIZEOF(.data);

    ASSERT(_flash_image_end <= ORIGIN(flash) + LENGTH(flash),
           "Flash region overflow")
    ASSERT(_heap_start <= _heap_end,
           "CL1-2512 DSRAM sections overlap the 4 KiB stack")
}
