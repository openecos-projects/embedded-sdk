#include "ecos/bsp/console.h"
#include "ecos/driver/interrupt.h"
#include "ecos/log.h"

#include <stddef.h>
#include <stdint.h>

#define LOG_TAG "plic"

static void plic_handler(void *arg)
{
    (void)arg;
}

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_interrupt_init(), "initialize PLIC");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_interrupt_alloc(3u, 1u, plic_handler, NULL),
        "allocate PLIC source"
    );
    if (ecos_interrupt_alloc(3u, 1u, plic_handler, NULL) != ECOS_ERR_BUSY)
        ECOS_PANIC(LOG_TAG, "duplicate PLIC allocation was accepted");
    ecos_interrupt_set_threshold(0u);
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_interrupt_enable(3u), "enable PLIC source"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_interrupt_disable(3u), "disable PLIC source"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_interrupt_free(3u), "free PLIC source"
    );
    (void)ECOS_LOGI(LOG_TAG, "PLIC source 3 allocation check passed");
    halt();
}
