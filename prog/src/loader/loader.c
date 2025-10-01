#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

// #define LD_YSYX2

extern uint32_t app_start;
extern uint32_t app_end;
extern uint32_t _appmem_start;
extern uint32_t _stack_top;
extern uint32_t _stack_pointer;

#define PROG_SIZE ((&app_end) - (&app_start))

#define APP_PMEM 0x80000000 // mem
// #define APP_PMEM 0x0F000000 // sram

int main()
{
    uint32_t *prog = (uint32_t *)&app_start;
    uint32_t *pmem = (uint32_t *)&(_appmem_start);
    // uintptr_t APP_PMEM;
    // if(*(uint32_t*)&_stack_top == 0x80000000){
    //     APP_PMEM=0x80000000; // mem
    // }else{
    //     APP_PMEM=0x0F000000; // sram
    // }
    putstr("Loading program of size: ");
    printf("%d bytes, expect 128 \'#\'\n", (uint32_t)PROG_SIZE * sizeof(uint32_t));
    putstr("Loading.....\n");
    uint32_t step = (uint32_t)(&app_end - &app_start) / 128;
    uint32_t *pre = prog;

    printf("stack top: [%x] stack pointer: [%x] pmem: [%x]\n", (uint32_t*)&_stack_top, (uint32_t*)&_stack_pointer, pmem);
    while (prog < &app_end)
    {
        *pmem++ = *prog++;
        // *pmem = *prog;
        // printf("[%x]: %x <=== %x\n", pmem, *pmem, *prog);
        // ++pmem, ++prog;

        if ((uint32_t)(prog - pre) >= step)
        {
            putch('#');
            pre = prog;
        }
    }
    putstr("\nLoad finished\nExec app...\n");
    printf("start addr is %x\n", APP_PMEM);
    asm volatile("fence.i");
    int (*f)() = (int (*)())(APP_PMEM);
    f();
    return 0;
}
