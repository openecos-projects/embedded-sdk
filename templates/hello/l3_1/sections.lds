ENTRY(_start)

SECTIONS {
  /* _pmem_start and _entry_offset are defined in LDFLAGS */
  _pmem_start = 0x30000000;
  _entry_offset = 0x0;
  . = _pmem_start + _entry_offset;
  .text : {
    *(entry)
    *(.text*)
  }
  etext = .;
  _etext = .;
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data)
  }
  edata = .;
  _data = .;
  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(0x80000000);
  _appmem_start = _stack_top,
  . = _stack_top + 0x100000;
  _stack_pointer = .;
  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
}
