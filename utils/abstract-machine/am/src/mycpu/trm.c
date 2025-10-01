#include <am.h>
#include <klib-macros.h>
#include <uart.h>

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

#define SOC_ENV
// NOTE: comment for soc prj, because soc need uart to putch
void putch(char ch)
{
#ifdef SOC_ENV
  drv_uart_putc(ch);
#else
  asm volatile("mv a0, %0; .word 0x0000007b"
               :
               : "r"(ch));
#endif
}

void halt(int code)
{
#ifdef SOC_ENV
  asm volatile("nop");
#else
  asm volatile("mv a0, %0; .word 0x0000006b"
               :
               : "r"(code));
#endif
  while (1)
    ;
}

#define TIMER0_BASE_ADDR  0x10009000
#define TIMER0_REG_CTRL  *((volatile uint32_t *)(TIMER0_BASE_ADDR + 0))
#define TIMER0_REG_PSCR  *((volatile uint32_t *)(TIMER0_BASE_ADDR + 4))
#define TIMER0_REG_CNT   *((volatile uint32_t *)(TIMER0_BASE_ADDR + 8))
#define TIMER0_REG_CMP   *((volatile uint32_t *)(TIMER0_BASE_ADDR + 12))
#define TIMER0_REG_STAT  *((volatile uint32_t *)(TIMER0_BASE_ADDR + 16))



void _trm_init()
{
#ifdef SOC_ENV
  // virt_uart_init(115200, 25000000);
  // volatile uint32_t val = 10000;
  // while(--val);
  // TIMER0_REG_CTRL = (uint32_t)0x0;
  // while(TIMER0_REG_STAT == 1); // clear irq
  // TIMER0_REG_PSCR = 0;
  // TIMER0_REG_CMP  = 99999;

  // TIMER0_REG_CTRL = (uint32_t)0xD;
  // for(int i = 0; i < 1; ++i) {
      // while(TIMER0_REG_STAT == 0);
  // }
  // TIMER0_REG_CTRL = (uint32_t)0x0;

  virt_uart_init();
  // virt_uart_init(9600, 20000000);
  // virt_uart_init(9600, 15000000);
  // virt_uart_init(14400, 20000000);
  // virt_uart_init(115200, 8000000);
  // virt_uart_init(9600, 8000000);
  // virt_uart_init(9600, 4000000);
  // virt_uart_init(9600, 1000000);

#endif
  int ret = main(mainargs);
  halt(ret);
}
