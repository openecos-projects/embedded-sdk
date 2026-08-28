ENTRY(_start)

MEMORY {
    flash : ORIGIN = 0x30000000, LENGTH = 16M,
    psram : ORIGIN = 0xC0000000, LENGTH = 8M
}

SECTIONS {
    . = ORIGIN(flash);

    .fsbl : ALIGN(4) {
        KEEP(*(entry))
        KEEP(*(entry.boot))
    } > flash

    /* LOADADDR 获取加载地址 */
    _ssbl_start = LOADADDR(.ssbl);
    .ssbl : ALIGN(4) {
        _ssbl_op = .;
        KEEP(*(ssbl.boot))
        KEEP(*(ssbl*))
        . = ALIGN(4);
        _ssbl_ed = .;
    } > psram AT> flash

    _text_start = LOADADDR(.text);
    .text : ALIGN(4) {
        _text_op = .;
        KEEP(*(.text.startup.main))
        KEEP(*(.text.main))
        *(.text .text.*)
        . = ALIGN(4);
        _text_ed = .;
    } > psram AT> flash
    ASSERT(_text_op == main, "PSRAM entry must point to main")

    _rodata_start = LOADADDR(.rodata);
    .rodata : ALIGN(4) {
        _rodata_op = .;
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        *(.eh_frame*)
        . = ALIGN(4);
        _rodata_ed = .;
    } > psram AT> flash

    _data_start = LOADADDR(.data);
    .data : ALIGN(4) {
        _data_op = .;
        _sdata = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
        _edata = .;
        _data_ed = .;
    } > psram AT> flash
    _sidata = _data_start;

    .bss (NOLOAD) : ALIGN(4) {
        _bss_op = .;
        _sbss = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        _bss_ed = .;
    } > psram

    _stack_top = ALIGN(0x1000);
    . = _stack_top + 0x100000;
    _stack_pointer = .;
    _heap_start = ALIGN(0x1000);
}
