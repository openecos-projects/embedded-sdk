/* 定义 StartySky T1-Pico 汇编固件的 XFlash XIP 布局。 */
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    xflash (rx) : ORIGIN = 0x20000000, LENGTH = 16M
}

PHDRS
{
    text PT_LOAD FLAGS(5);
}

SECTIONS
{
    /* 将复位入口、代码和只读数据全部放入 XFlash。 */
    . = ORIGIN(xflash);
    .text : ALIGN(4)
    {
        KEEP(*(.text.start))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > xflash :text

    /* 记录镜像末尾并在链接阶段检查 XFlash 边界。 */
    . = ALIGN(4);
    _xflash_image_end = .;
    ASSERT(_xflash_image_end <= ORIGIN(xflash) + LENGTH(xflash),
           "xflash region overflow")
}
