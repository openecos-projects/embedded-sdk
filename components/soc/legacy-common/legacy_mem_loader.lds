ENTRY(_start)

MEMORY {
    flash (rx) : ORIGIN = 0x30000000, LENGTH = 16M
}

SECTIONS {
    .text : ALIGN(4) {
        KEEP(*(entry))
        *(.text .text.*)
    } > flash

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
    } > flash

    .data : ALIGN(4) {
        *(.data .data.*)
    } > flash

    .bss (NOLOAD) : ALIGN(4) {
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
    } > flash

    . = ALIGN(16);
    . += 0x4000;
    _loader_stack_pointer = .;
}
