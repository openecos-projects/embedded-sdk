OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    flash (rx) : ORIGIN = 0x20000000, LENGTH = 16M
}

SECTIONS
{
    . = ORIGIN(flash);
    .text : ALIGN(4)
    {
        KEEP(*(.text.start))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > flash
}
