#include <stdint.h>
#include "string.h"
#include "stdio.h"
#include "generated/autoconf.h"

extern uint32_t app_start;
extern uint32_t app_end;

#define APP_ENTRY CONFIG_LINK_ADDRESS

void main() {
    hal_sys_uart_init();

    uint32_t *src = (uint32_t *)&app_start;
    uint32_t *dest = (uint32_t *)APP_ENTRY;
    uint32_t *end = (uint32_t *)&app_end;
    // 计算总字节数
    uint32_t total = (uint32_t)((uintptr_t)&app_end - (uintptr_t)&app_start);
    uint32_t copied = 0;
    uint32_t percent = 0;
    uint32_t last_percent = 0;
    uint32_t *pre = src;
    
    // 打印起始进度
    // sys_putstr("Loading:");
    hal_sys_putchar('L');
    hal_sys_putchar('O');
    hal_sys_putchar('A');
    hal_sys_putchar('D');
    hal_sys_putchar('I');
    hal_sys_putchar('N');
    hal_sys_putchar('G');
    hal_sys_putchar(':');
    hal_sys_putchar(' ');

    uint32_t step = (uint32_t)(&app_end - &app_start) / 128;
    // Copy payload to RAM
    while (src < end) {
        *dest++ = *src++;
        copied += sizeof(uint32_t);
        if ((uint32_t)(src - pre) >= step){
            hal_sys_putchar('#');
            pre = src;
        }
    }
    hal_sys_putchar('\r');
    hal_sys_putchar('\n');
    

    // Flush instruction cache (using fence.i instruction)
    asm volatile("fence.i");
    
    // Jump to app
    void (*app_entry)() = (void (*)())APP_ENTRY;
    app_entry();
    
}
