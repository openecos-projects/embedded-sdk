#include "ecos/board_resources.h"
#include "ecos/bsp/console.h"
#include "ecos/log.h"
#include "ecos/soft_spi.h"

#if !ECOS_BOARD_HAS_SOFT_SPI_BUS
#error "soft-spi-jedec-id requires a Board soft-spi-bus resource"
#endif

#define LOG_TAG "soft-spi"

int main(void)
{
    ecos_soft_spi_t bus = {0};
    const ecos_soft_spi_config_t config = {
        .chip_select = ECOS_BOARD_SOFT_SPI_CHIP_SELECT,
        .clock = ECOS_BOARD_SOFT_SPI_CLOCK,
        .mosi = ECOS_BOARD_SOFT_SPI_MOSI,
        .miso = ECOS_BOARD_SOFT_SPI_MISO,
        .mode = (ecos_soft_spi_mode_t)ECOS_BOARD_SOFT_SPI_MODE,
    };
    uint8_t ignored;
    uint8_t manufacturer;
    uint8_t memory_type;
    uint8_t capacity;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_soft_spi_init(&bus, &config), "initialize SPI");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_soft_spi_start(&bus), "select flash");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_soft_spi_transfer_byte(&bus, 0x9fu, &ignored), "send JEDEC command"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_soft_spi_transfer_byte(&bus, 0u, &manufacturer), "read manufacturer"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_soft_spi_transfer_byte(&bus, 0u, &memory_type), "read memory type"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_soft_spi_transfer_byte(&bus, 0u, &capacity), "read capacity"
    );
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_soft_spi_stop(&bus), "deselect flash");

    (void)ECOS_LOGI(LOG_TAG, "JEDEC ID: %02x %02x %02x", manufacturer,
                    memory_type, capacity);
    return 0;
}
