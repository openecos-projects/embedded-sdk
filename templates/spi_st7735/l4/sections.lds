ENTRY(_start)

MEMORY {
  ROM (rx)  : ORIGIN = 0x30000000, LENGTH = 16M
  SRAM (rwx) : ORIGIN = 0x02020000, LENGTH = 64K
  MEM (rwx) : ORIGIN = 0xc0000000, LENGTH = 8M
}

#ifdef CONFIG_LINK_TARGET_XIP
  #define REGION_TEXT ROM
  #define REGION_RODATA ROM
  #define REGION_DATA MEM
  #define REGION_BSS MEM
  #define REGION_STACK SRAM
#else
  #define REGION_TEXT MEM
  #define REGION_RODATA MEM
  #define REGION_DATA MEM
  #define REGION_BSS MEM
  #define REGION_STACK MEM
#endif

SECTIONS {
  .text : {
    . = ALIGN(4);
    *(entry)
    *(.text .text.*)
  } > REGION_TEXT

  .rodata : {
    . = ALIGN(4);
    *(.rodata .rodata.*)
  } > REGION_RODATA

  .data : {
    . = ALIGN(4);
    _sdata = .;
    *(.data .data.*)
    *(.sdata .sdata.*)
    . = ALIGN(4);
    _edata = .;
  } > REGION_DATA AT > REGION_RODATA

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) : {
    . = ALIGN(4);
    _sbss = .;
    *(.bss .bss.*)
    *(.sbss .sbss.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > REGION_BSS

  _heap_start = ALIGN(0x1000);

  .stack (NOLOAD) : {
    . = ALIGN(16);
    . += 0x10000; /* 64KB stack */
    _stack_pointer = .;
  } > REGION_STACK
}
