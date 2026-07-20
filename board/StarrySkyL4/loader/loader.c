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

typedef void (*voidfunc)();


__attribute__((section("entry.boot"))) void _first_bootloader()
{
    uint8_t *dst = (uint8_t*)&_ssbl_op;
    const uint8_t *src = (uint8_t*)&_ssbl_start;
    uint32_t len = (uintptr_t)&_ssbl_ed - (uintptr_t)&_ssbl_op;

    uint32_t *d = (uint32_t *)dst;
    const uint32_t *s = (uint32_t *)src;
    uintptr_t cnt32 = len / 4;

    while (cnt32--) *d++ = *s++;

    dst = (uint8_t *)d;
    src = (const uint8_t *)s;
    uintptr_t rem = len % 4;

    while (rem--) *dst++ = *src++;

    voidfunc ssbl = (voidfunc)(&_ssbl_op);
    ssbl();
}


__attribute__((section("ssbl"))) void loader(uint8_t* dst, const uint8_t* src, uintptr_t len)
{
    uint32_t *d = (uint32_t *)dst;
    const uint32_t *s = (uint32_t *)src;
    uintptr_t cnt32 = len / 4;

    while (cnt32--) *d++ = *s++;

    dst = (uint8_t *)d;
    src = (const uint8_t *)s;
    uintptr_t rem = len % 4;

    while (rem--) *dst++ = *src++;
}


__attribute__((section("ssbl"))) void zero(uint8_t *dst, uintptr_t len)
{
    uint32_t *d = (uint32_t *)dst;
    uintptr_t cnt32 = len / 4;

    while (cnt32--) *d++ = 0u;

    dst = (uint8_t *)d;
    uintptr_t rem = len % 4;

    while (rem--) *dst++ = 0u;
}


__attribute__((section("ssbl.boot"))) void _second_bootloader()
{
    // 代码加载
    uint8_t *d = (uint8_t*)&_text_op;
    const uint8_t *s = (uint8_t*)&_text_start;
    uint32_t n = (uintptr_t)&_text_ed - (uintptr_t)&_text_op;
    loader(d, s, n);

    // 只读全局变量加载
    d = (uint8_t*)&_rodata_op;
    s = (uint8_t*)&_rodata_start;
    n = (uintptr_t)&_rodata_ed - (uintptr_t)&_rodata_op;
    loader(d, s, n);

    // 全局变量加载
    d = (uint8_t*)&_data_op;
    s = (uint8_t*)&_data_start;
    n = (uintptr_t)&_data_ed - (uintptr_t)&_data_op;
    loader(d, s, n);

    // 未初始化全局变量清零
    d = (uint8_t*)&_bss_op;
    n = (uintptr_t)&_bss_ed - (uintptr_t)&_bss_op;
    zero(d, n);

    voidfunc start = (voidfunc)(&_text_op);

    start();
}
