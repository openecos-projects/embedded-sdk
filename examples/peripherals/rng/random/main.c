#include "ecos/bsp/console.h"
#include "ecos/driver/rng.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "rng-random"
#define RANDOM_WORD_COUNT 8u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    const ecos_rng_config_t config = ECOS_RNG_CONFIG_DEFAULT;
    unsigned index;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_rng_init(ECOS_RNG_DEFAULT, &config), "initialize RNG"
    );

    /* 播种后给硬件若干周期再读取第一个值。 */
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 1u),
        "delay after seeding"
    );

    for (index = 0u; index < RANDOM_WORD_COUNT; ++index) {
        uint32_t value;

        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ecos_rng_read(ECOS_RNG_DEFAULT, &value), "read random word"
        );
        (void)ECOS_LOGI(LOG_TAG, "random[%u] = 0x%08X", index, value);
    }

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_rng_deinit(ECOS_RNG_DEFAULT), "deinitialize RNG"
    );
    halt();
}
