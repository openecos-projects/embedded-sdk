ENTRY(_start)

MEMORY {
    FLASH (rx) : ORIGIN = 0x30000000, LENGTH = 16M
}

PHDRS {
    text PT_LOAD FLAGS(5);
}

SECTIONS {
    . = ORIGIN(FLASH);

    .text : ALIGN(4) {
        KEEP(*(entry))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > FLASH :text
}
