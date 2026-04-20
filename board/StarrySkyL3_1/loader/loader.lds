ENTRY(_start)

SECTIONS {
  _pmem_start = 0x30000000;
  . = _pmem_start;
  .text : {
    *(entry)
    *(.text*)
  }
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data)
  }
  .bss : {
    *(.bss*)
    *(.sbss*)
  }
  
  /* Loader stack */
  . = ALIGN(16);
  . = . + 0x4000;
  _stack_pointer = .;
}
