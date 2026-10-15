ENTRY(_start)

MEMORY {
    flash (rx)  : ORIGIN = 0x00000000, LENGTH = 16M
    ram   (rwx) : ORIGIN = 0x40000000, LENGTH = 8M
    exec_ram (rwx) : ORIGIN = 0x30000000, LENGTH = 128K
}

SECTIONS {
    . = ORIGIN(flash);
    .text : ALIGN(4) {
        KEEP(*(.text.startup))
        *(.text .text.*)
    } > flash
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    } > flash
    _sidata = LOADADDR(.data);
    .data : ALIGN(4) {
        _sdata = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
        _edata = .;
    } > ram AT> flash
    .bss (NOLOAD) : ALIGN(4) {
        _sbss = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > ram
    .fcc_exec (NOLOAD) : ALIGN(16) {
        KEEP(*(.fcc_exec))
        . = ALIGN(16);
    } > exec_ram
    . = ALIGN(16);
    _heap_start = .;
    _stack_pointer = ORIGIN(ram) + LENGTH(ram) - 16;
}
