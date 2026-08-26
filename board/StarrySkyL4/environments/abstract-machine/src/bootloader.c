#include <stdint.h>

extern char _ssbl_start;
extern char _ssbl_op;
extern char _ssbl_ed;
extern char _text_start;
extern char _text_op;
extern char _text_ed;
extern char _rodata_start;
extern char _rodata_op;
extern char _rodata_ed;
extern char _data_start;
extern char _data_op;
extern char _data_ed;
extern char _bss_op;
extern char _bss_ed;

typedef void (*entry_fn_t)(void);


/**
 * 在 Flash 阶段复制一个内存区域。
 */
__attribute__((section("entry.boot")))
static void fsbl_copy(uint8_t *dst, const uint8_t *src, uintptr_t len)
{
    /* 优先按 32 位宽度复制主体。 */
    while (len >= sizeof(uint32_t))
    {
        *(uint32_t *)dst = *(const uint32_t *)src;
        dst += sizeof(uint32_t);
        src += sizeof(uint32_t);
        len -= sizeof(uint32_t);
    }

    /* 复制剩余不足一个字的尾部。 */
    while (len-- != 0u)
        *dst++ = *src++;
}


/**
 * 将二级 loader 搬运到 PSRAM 并跳转执行。
 */
__attribute__((section("entry.boot")))
void _first_bootloader(void)
{
    uintptr_t len;

    /* 在调用任何 PSRAM 函数前完成二级 loader 搬运。 */
    len = (uintptr_t)&_ssbl_ed - (uintptr_t)&_ssbl_op;
    fsbl_copy((uint8_t *)&_ssbl_op, (const uint8_t *)&_ssbl_start, len);

    /* 跳转到已搬运的二级 loader。 */
    ((entry_fn_t)&_ssbl_op)();
}


/**
 * 在 PSRAM 阶段复制一个加载段。
 */
__attribute__((section("ssbl")))
static void ssbl_copy(uint8_t *dst, const uint8_t *src, uintptr_t len)
{
    /* 优先按 32 位宽度复制主体。 */
    while (len >= sizeof(uint32_t))
    {
        *(uint32_t *)dst = *(const uint32_t *)src;
        dst += sizeof(uint32_t);
        src += sizeof(uint32_t);
        len -= sizeof(uint32_t);
    }

    /* 复制剩余不足一个字的尾部。 */
    while (len-- != 0u)
        *dst++ = *src++;
}


/**
 * 在 PSRAM 阶段清零一个内存区域。
 */
__attribute__((section("ssbl")))
static void ssbl_zero(uint8_t *dst, uintptr_t len)
{
    /* 优先按 32 位宽度清零主体。 */
    while (len >= sizeof(uint32_t))
    {
        *(uint32_t *)dst = 0u;
        dst += sizeof(uint32_t);
        len -= sizeof(uint32_t);
    }

    /* 清零剩余不足一个字的尾部。 */
    while (len-- != 0u)
        *dst++ = 0u;
}


/**
 * 搬运 AM 运行段、清零 BSS 并进入 TRM。
 */
__attribute__((section("ssbl.boot")))
void _second_bootloader(void)
{
    uintptr_t len;

    /* 搬运代码段。 */
    len = (uintptr_t)&_text_ed - (uintptr_t)&_text_op;
    ssbl_copy((uint8_t *)&_text_op, (const uint8_t *)&_text_start, len);

    /* 搬运只读数据段。 */
    len = (uintptr_t)&_rodata_ed - (uintptr_t)&_rodata_op;
    ssbl_copy((uint8_t *)&_rodata_op, (const uint8_t *)&_rodata_start, len);

    /* 搬运可写数据段。 */
    len = (uintptr_t)&_data_ed - (uintptr_t)&_data_op;
    ssbl_copy((uint8_t *)&_data_op, (const uint8_t *)&_data_start, len);

    /* 清零未初始化数据后进入 TRM。 */
    len = (uintptr_t)&_bss_ed - (uintptr_t)&_bss_op;
    ssbl_zero((uint8_t *)&_bss_op, len);
    ((entry_fn_t)&_text_op)();
}
