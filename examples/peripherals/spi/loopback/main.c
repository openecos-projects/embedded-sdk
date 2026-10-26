#include "ecos/board_resources.h"
#include "ecos/bsp/console.h"
#include "ecos/driver/spi.h"
#include "ecos/log.h"

#include <stdint.h>

#define LOG_TAG "spi-loopback"

#if !ECOS_BOARD_HAS_SPI_BUS
#error "spi-loopback requires the selected Board to provide spi-bus"
#endif

int main(void)
{
    const ecos_spi_config_t config = {
        ECOS_BOARD_SPI_BUS_CLOCK_HZ,
        ECOS_BOARD_SPI_BUS_MODE,
        ECOS_BOARD_SPI_BUS_BIT_ORDER,
        ECOS_BOARD_SPI_BUS_CS_PORT,
        ECOS_BOARD_SPI_BUS_CS_PIN,
    };
    static const uint8_t pattern[] = { 0x55u, 0xAAu, 0x12u, 0xEDu };
    uint8_t received[sizeof(pattern)];
    size_t index;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_spi_init(ECOS_SPI_DEFAULT, &config), "initialize SPI1"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_spi_select(ECOS_SPI_DEFAULT), "select SPI device"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_spi_transfer(ECOS_SPI_DEFAULT, pattern, received, sizeof(pattern)),
        "transfer SPI loopback pattern"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_spi_deselect(ECOS_SPI_DEFAULT), "deselect SPI device"
    );

    for (index = 0u; index < sizeof(pattern); ++index) {
        if (received[index] != pattern[index]) {
            (void)ECOS_LOGE(
                LOG_TAG,
                "loopback mismatch at %u: tx=0x%02X rx=0x%02X",
                (unsigned)index, pattern[index], received[index]
            );
            for (;;)
                __asm__ volatile("nop");
        }
    }
    (void)ECOS_LOGI(LOG_TAG, "SPI1 loopback PASS: %u bytes", (unsigned)sizeof(pattern));
    for (;;)
        __asm__ volatile("nop");
}
