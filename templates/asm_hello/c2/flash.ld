ENTRY(_start)

MEMORY {
    FLASH(rx): ORIGIN = 0x00000000, LENGTH = 0x1000000
}

SECTIONS {
    .text : {
        *(entry)
        *(.text*)
    } > FLASH
} 