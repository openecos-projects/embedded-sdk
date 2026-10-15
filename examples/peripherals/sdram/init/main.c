#include "ecos/bsp/console.h"
#include "ecos/driver/sdram.h"
#include "ecos/heap.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "sdram-init"
#define TEST_WORD_COUNT 1024u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    volatile uint32_t *words;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_sdram_init(), "initialize SDRAM");
    words = ecos_heap_alloc(ECOS_HEAP_ZONE_EXTERNAL,
                            TEST_WORD_COUNT * sizeof(uint32_t));
    if (words == NULL)
        ECOS_PANIC(LOG_TAG, "external memory allocation failed");

    for (uint32_t index = 0u; index < TEST_WORD_COUNT; ++index)
        words[index] = 0xA5A50000u ^ index;
    for (uint32_t index = 0u; index < TEST_WORD_COUNT; ++index) {
        if (words[index] != (0xA5A50000u ^ index))
            ECOS_PANIC(LOG_TAG, "SDRAM mismatch at word %u", index);
    }
    ecos_heap_free((void *)words);
    (void)ECOS_LOGI(LOG_TAG, "SDRAM 4 KiB read/write check passed");
    halt();
}
