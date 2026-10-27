#include "ecos/bsp/console.h"
#include "ecos/driver/dma.h"
#include "ecos/log.h"

#define LOG_TAG "dma-memory-copy"
static uint8_t source[4] = { 1u, 2u, 3u, 4u };
static uint8_t destination[4];

int main(void)
{
    const ecos_dma_config_t config = {
        (uintptr_t)source, (uintptr_t)destination, 4u,
        ECOS_DMA_DIRECTION_MEMORY_TO_MEMORY,
        ECOS_DMA_WIDTH_BYTE, ECOS_DMA_WIDTH_BYTE, ECOS_DMA_MODE_NORMAL,
        ECOS_DMA_PRIORITY_LOW, 1u, 1u, 0u, 0u, 0u
    };
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_dma_init(0u, &config), "initialize DMA");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_dma_start(0u), "start DMA");
    (void)ECOS_LOGI(LOG_TAG, "DMA configured; completion requires hardware flag/IRQ");
    for (;;) __asm__ volatile("nop");
}
