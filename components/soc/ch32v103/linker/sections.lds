ENTRY(_start)

/* CH32V103C8T6: boot Flash alias, 64 KiB Flash and 20 KiB SRAM.
 * Matches the WCH CH32V103 EVT memory layout. */
MEMORY
{
    flash (rx) : ORIGIN = 0x00000000, LENGTH = 64K
    sram (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    .text : ALIGN(4)
    {
        KEEP(*(.text.start))
        *(.text .text.*)
        *(.rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4);
    } > flash

    .data : ALIGN(4)
    {
        _data_start = .;
        *(.data .data.* .sdata .sdata.*)
        . = ALIGN(4);
        _data_end = .;
    } > sram AT> flash
    _data_load = LOADADDR(.data);

    .bss (NOLOAD) : ALIGN(4)
    {
        _bss_start = .;
        *(.bss .bss.* .sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > sram

    _stack_pointer = ORIGIN(sram) + LENGTH(sram);
    _stack_bottom = _stack_pointer - 2048;
    _flash_image_start = ORIGIN(flash);
    _flash_image_end = LOADADDR(.data) + SIZEOF(.data);

    ASSERT(_bss_end <= _stack_bottom, "CH32V103 SRAM overflow: reserve 2 KiB stack")
    ASSERT(_flash_image_end <= ORIGIN(flash) + LENGTH(flash), "CH32V103 Flash overflow")
}
