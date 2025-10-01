ENTRY(_start)

SECTIONS {
    . = 0x30000000;
    .text : {
        *(entry)
        *(.text*)
    }
}