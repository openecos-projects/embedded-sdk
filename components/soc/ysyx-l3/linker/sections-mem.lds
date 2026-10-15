ENTRY(_start)

MEMORY {
    mem (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
    . = ORIGIN(mem);
    .text : ALIGN(8) {
        KEEP(*(.text.startup))
        *(.text .text.*)
    } > mem
    .rodata : ALIGN(8) {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    } > mem
    _sidata = LOADADDR(.data);
    .data : ALIGN(8) {
        _sdata = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(8);
        _edata = .;
    } > mem
    .bss (NOLOAD) : ALIGN(8) {
        _sbss = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(8);
        _ebss = .;
    } > mem
    . = ALIGN(16);
    _heap_start = .;
    _stack_pointer = ORIGIN(mem) + LENGTH(mem) - 16;
}
