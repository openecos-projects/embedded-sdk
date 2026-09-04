#include <stdint.h>

extern uint8_t _ssbl_start;
extern uint8_t _ssbl_op;
extern uint8_t _ssbl_ed;

extern uint8_t _text_start;
extern uint8_t _text_op;
extern uint8_t _text_ed;

extern uint8_t _rodata_start;
extern uint8_t _rodata_op;
extern uint8_t _rodata_ed;

extern uint8_t _data_start;
extern uint8_t _data_op;
extern uint8_t _data_ed;

extern uint8_t _bss_op;
extern uint8_t _bss_ed;

typedef void (*entry_function_t)(void);

/**
 * 将二级加载器从 Flash 搬运到 SDRAM 并转交执行。
 */
__attribute__((section(".fsbl.boot"), noinline)) void _first_bootloader(void)
{
    /* 初始化二级加载器的源地址、目标地址、长度和入口。 */
    uint8_t *destination = &_ssbl_op;
    const uint8_t *source = &_ssbl_start;
    uintptr_t length = (uintptr_t)&_ssbl_ed - (uintptr_t)&_ssbl_op;
    entry_function_t second_bootloader;

    /* 将二级加载器逐字节搬运到 SDRAM。 */
    while (length > 0u)
    {
        *destination++ = *source++;
        length--;
    }

    /* 等待数据写入完成并刷新后续取指视图。 */
    __asm__ volatile("fence rw, rw" : : : "memory");
    __asm__ volatile("fence.i" : : : "memory");

    /* 跳转到 SDRAM 中的二级加载器。 */
    second_bootloader = (entry_function_t)&_ssbl_op;
    second_bootloader();
}


/**
 * 将指定运行段从 Flash 逐字节搬运到 SDRAM。
 */
__attribute__((section(".ssbl"), noinline)) static void load_section(uint8_t *destination,
                                                                     const uint8_t *source,
                                                                     uintptr_t length)
{
    /* 持续搬运数据，直到指定运行段全部复制完成。 */
    while (length > 0u)
    {
        *destination++ = *source++;
        length--;
    }
}


/**
 * 搬运应用运行段、清零未初始化数据并进入主程序。
 */
__attribute__((section(".ssbl.boot"), noinline)) void _second_bootloader(void)
{
    /* 保存当前运行段长度、清零地址和应用入口。 */
    uintptr_t length;
    uint8_t *clear_address;
    entry_function_t application_entry;

    /* 将可执行代码段从 Flash 搬运到 SDRAM。 */
    length = (uintptr_t)&_text_ed - (uintptr_t)&_text_op;
    load_section(&_text_op, &_text_start, length);

    /* 将只读数据段从 Flash 搬运到 SDRAM。 */
    length = (uintptr_t)&_rodata_ed - (uintptr_t)&_rodata_op;
    load_section(&_rodata_op, &_rodata_start, length);

    /* 将可写数据段从 Flash 搬运到 SDRAM。 */
    length = (uintptr_t)&_data_ed - (uintptr_t)&_data_op;
    load_section(&_data_op, &_data_start, length);

    /* 将未初始化数据段逐字节清零。 */
    clear_address = &_bss_op;
    while (clear_address < &_bss_ed)
        *clear_address++ = 0u;

    /* 等待数据写入完成并刷新后续取指视图。 */
    __asm__ volatile("fence rw, rw" : : : "memory");
    __asm__ volatile("fence.i" : : : "memory");

    /* 跳转到 SDRAM 中的应用程序入口。 */
    application_entry = (entry_function_t)&_text_op;
    application_entry();
}
