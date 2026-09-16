#include "ecos/board_resources.h"
#include "ecos/bsp/console.h"
#include "ecos/device/st7735.h"
#include "ecos/driver/timer.h"
#include "ecos/error.h"
#include "ecos/log.h"

#define LOG_TAG "st7735"

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

int main(void)
{
    ecos_st7735_t display;
    ecos_st7735_config_t display_config = {
        ECOS_BOARD_QSPI_BUS_CONTROLLER,
        ECOS_BOARD_DISPLAY_CHIP_SELECT,
        ECOS_BOARD_DISPLAY_DC_PORT,
        ECOS_BOARD_DISPLAY_DC_PIN,
        ECOS_BOARD_DISPLAY_RESET_PORT,
        ECOS_BOARD_DISPLAY_RESET_PIN,
        ECOS_BOARD_DISPLAY_BACKLIGHT_PORT,
        ECOS_BOARD_DISPLAY_BACKLIGHT_PIN,
        ECOS_BOARD_DISPLAY_WIDTH,
        ECOS_BOARD_DISPLAY_HEIGHT,
        ECOS_BOARD_DISPLAY_ROTATION,
        ECOS_BOARD_DISPLAY_HORIZONTAL_OFFSET,
        ECOS_BOARD_DISPLAY_VERTICAL_OFFSET,
        ECOS_BOARD_QSPI_BUS_CLOCK_DIVIDER,
    };

#if !ECOS_BOARD_HAS_DISPLAY || !ECOS_BOARD_HAS_QSPI_BUS
#error "This example requires a board display backed by a QSPI bus"
#endif

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_st7735_init(&display, &display_config),
        "initialize ST7735"
    );

    (void)ECOS_LOGI(LOG_TAG, "ST7735 %ux%u initialized over QSPI",
                    display_config.width, display_config.height);
    for (;;) {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_st7735_fill(&display, 0u, 0u, display_config.width,
                             display_config.height, 0xF800F800u),
            "fill red"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 250u),
            "delay after red"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_st7735_fill(&display, 0u, 0u, display_config.width,
                             display_config.height, 0x07E007E0u),
            "fill green"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 250u),
            "delay after green"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_st7735_fill(&display, 0u, 0u, display_config.width,
                             display_config.height, 0x001F001Fu),
            "fill blue"
        );
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 250u),
            "delay after blue"
        );
    }

    halt();
}
