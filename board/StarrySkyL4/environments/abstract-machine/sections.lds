ENTRY(_start)

MEMORY {
    flash : ORIGIN = 0x30000000, LENGTH = 16M,
    psram : ORIGIN = 0xC0000000, LENGTH = 8M
}

PHDRS {
    fsbl PT_LOAD FLAGS(5);
    ssbl PT_LOAD FLAGS(5);
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
    bss PT_LOAD FLAGS(6);
}

SECTIONS {
    . = ORIGIN(flash);

    .fsbl : ALIGN(4) {
        KEEP(*(entry))
        KEEP(*(entry.boot))
    } > flash :fsbl

    _ssbl_start = LOADADDR(.ssbl);
    .ssbl : ALIGN(4) {
        _ssbl_op = .;
        KEEP(*(ssbl.boot))
        KEEP(*(ssbl*))
        . = ALIGN(4);
        _ssbl_ed = .;
    } > psram AT> flash :ssbl

    _text_start = LOADADDR(.text);
    .text : ALIGN(4) {
        _text_op = .;
        KEEP(*(.text._trm_init))
        *(.text .text.*)
        . = ALIGN(4);
        _text_ed = .;
    } > psram AT> flash :text
    ASSERT(_text_op == _trm_init, "AM text entry must point to _trm_init")

    _rodata_start = LOADADDR(.rodata);
    .rodata : ALIGN(4) {
        _rodata_op = .;
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        *(.eh_frame*)
        . = ALIGN(4);
        _rodata_ed = .;
    } > psram AT> flash :rodata

    _data_start = LOADADDR(.data);
    .data : ALIGN(4) {
        _data_op = .;
        _sdata = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
        _edata = .;
        _data_ed = .;
    } > psram AT> flash :data
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
    } > psram :bss

    _stack_top = ALIGN(0x1000);
    . = _stack_top + 0x100000;
    _stack_pointer = .;
    _heap_start = ALIGN(0x1000);
    _heap_end = ORIGIN(psram) + LENGTH(psram);

    ASSERT(LOADADDR(.data) + SIZEOF(.data) <= ORIGIN(flash) + LENGTH(flash),
           "AM image exceeds L4 Flash")
    ASSERT(_bss_ed <= ORIGIN(psram) + LENGTH(psram),
           "AM sections exceed L4 PSRAM")
    ASSERT(_stack_top >= _bss_ed, "AM stack overlaps BSS")
    ASSERT(_stack_pointer <= ORIGIN(psram) + LENGTH(psram),
           "AM stack exceeds L4 PSRAM")
    ASSERT(_stack_pointer <= _heap_start, "AM stack and heap overlap")
    ASSERT(_heap_start < _heap_end, "AM heap is empty")
}
