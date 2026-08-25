/* 定义 StartySky T1 固件的入口和可配置存储器布局。 */
ENTRY(_start)

#ifdef CONFIG_LINK_TARGET_SDRAM
MEMORY
{
    flash (rx)  : ORIGIN = 0x20000000, LENGTH = 16M
    sdram (rwx) : ORIGIN = 0x80000000, LENGTH = 32M
}

/* 将 Flash 引导代码、SDRAM 代码和 SDRAM 数据分配到独立程序头。 */
PHDRS
{
    fsbl PT_LOAD FLAGS(5);
    sdram_code PT_LOAD FLAGS(5);
    sdram_data PT_LOAD FLAGS(6);
}

SECTIONS
{
    /* 将复位入口、SDRAM 初始化和一级加载器保留在 Flash。 */
    .fsbl : ALIGN(4)
    {
        _fsbl_start = .;
        KEEP(*(.text.start))
        KEEP(*(.fsbl.boot))
        . = ALIGN(4);
        _fsbl_end = .;
    } > flash :fsbl

    /* 将二级加载器放入 SDRAM，并在 Flash 保存连续装载副本。 */
    _ssbl_start = LOADADDR(.ssbl);
    .ssbl : ALIGN(4)
    {
        _ssbl_op = .;
        KEEP(*(.ssbl.boot))
        KEEP(*(.ssbl .ssbl.*))
        . = ALIGN(4);
        _ssbl_ed = .;
    } > sdram AT> flash :sdram_code

    /* 将应用代码放入 SDRAM，并在 Flash 保存连续装载副本。 */
    _text_start = LOADADDR(.text);
    .text : ALIGN(4)
    {
        _text_op = .;
        KEEP(*(.text.startup.main))
        KEEP(*(.text.main))
        *(.text .text.*)
        . = ALIGN(4);
        _text_ed = .;
    } > sdram AT> flash :sdram_code

    /* 将只读数据放入 SDRAM，并在 Flash 保存连续装载副本。 */
    _rodata_start = LOADADDR(.rodata);
    .rodata : ALIGN(4)
    {
        _rodata_op = .;
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        *(.eh_frame .eh_frame.*)
        . = ALIGN(4);
        _rodata_ed = .;
    } > sdram AT> flash :sdram_code

    /* 将可写数据放入 SDRAM，并在 Flash 保存连续装载副本。 */
    _data_start = LOADADDR(.data);
    .data : ALIGN(4)
    {
        _data_op = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        *(.got .got.*)
        . = ALIGN(4);
        _data_ed = .;
    } > sdram AT> flash :sdram_data

    /* 在 SDRAM 中保留未初始化数据，并由二级加载器清零。 */
    .bss (NOLOAD) : ALIGN(4)
    {
        _bss_op = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_ed = .;
    } > sdram :sdram_data

    /* 在运行段之后预留 1 MiB 栈，其余 SDRAM 作为堆。 */
    . = ALIGN(0x1000);
    _stack_bottom = .;
    . = . + 0x100000;
    _stack_top = .;
    _stack_pointer = _stack_top;
    _heap_start = ALIGN(0x1000);
    _heap_end = ORIGIN(sdram) + LENGTH(sdram);
    _flash_image_start = ORIGIN(flash);
    _flash_image_end = LOADADDR(.data) + SIZEOF(.data);

    /* 在链接阶段检查入口、装载镜像和各运行存储器边界。 */
    ASSERT(_fsbl_start == ORIGIN(flash),
           "FSBL does not start at the Flash reset address")
    ASSERT(_ssbl_start == _fsbl_end,
           "FSBL and SSBL load images are not contiguous")
    ASSERT(_text_start == _ssbl_start + SIZEOF(.ssbl),
           "SSBL and text load images are not contiguous")
    ASSERT(_rodata_start == _text_start + SIZEOF(.text),
           "text and rodata load images are not contiguous")
    ASSERT(_data_start == _rodata_start + SIZEOF(.rodata),
           "rodata and data load images are not contiguous")
    ASSERT(_flash_image_end <= ORIGIN(flash) + LENGTH(flash),
           "Flash region overflow")
    ASSERT(_ssbl_op == ORIGIN(sdram),
           "SSBL does not start at the SDRAM base address")
    ASSERT(_text_op == main,
           "SDRAM application entry does not point to main")
    ASSERT(_bss_ed <= _stack_bottom,
           "SDRAM runtime sections overlap the stack")
    ASSERT(_stack_top - _stack_bottom == 0x100000,
           "SDRAM stack size is not 1 MiB")
    ASSERT(_stack_top <= ORIGIN(sdram) + LENGTH(sdram),
           "SDRAM stack exceeds the SDRAM boundary")
    ASSERT(_heap_start >= _stack_top,
           "SDRAM stack and heap overlap")
    ASSERT(_heap_start < _heap_end,
           "SDRAM heap is empty")
}
#else
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

    /* 保持原有堆起点，并把栈顶设置到 DSRAM 末端。 */
    . = ALIGN(16);
    _heap_start = .;
    _stack_bottom = .;
    _stack_top = ORIGIN(dsram) + LENGTH(dsram);
    _stack_pointer = _stack_top;
    _flash_image_start = ORIGIN(flash);
    _flash_image_end = LOADADDR(.data) + SIZEOF(.data);

    /* 在链接阶段阻止 Flash 越界或栈空间不足。 */
    ASSERT(_flash_image_start == ORIGIN(flash),
           "Flash image does not start at the reset address")
    ASSERT(_flash_image_end <= ORIGIN(flash) + LENGTH(flash),
           "Flash region overflow")
    ASSERT(_stack_bottom + 0x1000 <= _stack_top,
           "less than 4 KiB available for the DSRAM stack")
}
#endif
