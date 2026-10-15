#include <stdint.h>

#include "legacy_soc.h"

extern const uint8_t _payload_start[];
extern const uint8_t _payload_end[];

#define APP_ENTRY ((uintptr_t)0x80000000u)

static void loader_uart_init(void)
{
#if LEGACY_UART0_SIMPLE
    /* C1/C2 do not use the two-stage loader. Keep this branch for reuse. */
    REG_UART_0_CLKDIV = 0u;
#else
    REG_UART_0_LC = 0x80u;
    REG_UART_0_TH = 13u;
    REG_UART_0_IE = 0u;
    REG_UART_0_LC = 0x03u;
#endif
}

static void loader_uart_putc(char value)
{
#if LEGACY_UART0_SIMPLE
    REG_UART_0_DATA = (uint32_t)(uint8_t)value;
#else
    while ((REG_UART_0_LS & 0x20u) == 0u)
        ;
    REG_UART_0_TH = (uint8_t)value;
#endif
}

static void loader_uart_puts(const char *text)
{
    while (*text != '\0')
        loader_uart_putc(*text++);
}

void main(void)
{
    const uint8_t *source = _payload_start;
    const uint8_t *end = _payload_end;
    uint8_t *destination = (uint8_t *)APP_ENTRY;

    loader_uart_init();
    loader_uart_puts("LOADING: ");
    while (source < end) {
        *destination++ = *source++;
    }
    loader_uart_puts("\r\n");

    __asm__ volatile("fence.i" ::: "memory");
    ((void (*)(void))APP_ENTRY)();
    for (;;)
        ;
}
