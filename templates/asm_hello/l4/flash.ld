ENTRY(_start)

MEMORY {
    FLASH(rx): ORIGIN = 0x30000000, LENGTH = 0x1000000
}

PHDRS {
    text PT_LOAD FLAGS(5);
}

SECTIONS {
    .text : {
        *(entry)
        *(.text*)
        *(.rodata*)
    } > FLASH :text
}
