#include "ecos/bsp/console.h"
#include "ecos/driver/adc.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "adc-oneshot"

int main(void)
{
    const ecos_adc_config_t config = { 0u, 239u, 6u };
    uint16_t value;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_adc_init(ECOS_ADC_DEFAULT, &config), "initialize ADC1"
    );
    for (;;) {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ecos_adc_read(ECOS_ADC_DEFAULT, &value), "read ADC1"
        );
        (void)ECOS_LOGI(LOG_TAG, "ADC1 channel 0: %u", (unsigned)value);
        for (volatile uint32_t delay = 0u; delay < 1000000u; ++delay)
            __asm__ volatile("nop");
    }
}
