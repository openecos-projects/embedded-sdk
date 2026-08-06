/* 定义 StartySky T1 Flash XIP 固件的入口和存储器布局。 */
ENTRY(_start)

MEMORY
{
    flash (rx)  : ORIGIN = 0x20000000, LENGTH = 16M
    dsram (rwx) : ORIGIN = 0x01800000, LENGTH = 16K
}

SECTIONS
{
    /* 将启动入口、代码和只读数据放入 Flash。 */
    .text : ALIGN(4)
    {
        KEEP(*(.text.start))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > flash

    /* 将初始化数据的运行地址放入 DSRAM，并在 Flash 保存装载副本。 */
    _data_load = LOADADDR(.data);
    .data : ALIGN(4)
    {
        _data_start = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        _data_end = .;
    } > dsram AT> flash

    /* 在 DSRAM 中保留未初始化数据，并由启动代码清零。 */
    .bss (NOLOAD) : ALIGN(4)
    {
        _bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        _bss_end = .;
    } > dsram

    /* 将堆起点对齐，并把栈顶设置到 DSRAM 末端。 */
    . = ALIGN(16);
    _heap_start = .;
    _stack_bottom = .;
    _stack_top = ORIGIN(dsram) + LENGTH(dsram);
    _stack_pointer = _stack_top;
    _flash_image_end = LOADADDR(.data) + SIZEOF(.data);

    /* 在链接阶段阻止 Flash 越界或栈空间不足。 */
    ASSERT(_flash_image_end <= ORIGIN(flash) + LENGTH(flash),
           "flash region overflow")
    ASSERT(_stack_bottom + 0x1000 <= _stack_top,
           "less than 4 KiB available for the DSRAM stack")
}
