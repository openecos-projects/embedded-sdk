#include "ecos/board_resources.h"
#include "ecos/bsp/display.h"
#include "ecos/device/st7735.h"

#if !ECOS_BOARD_HAS_DISPLAY || !ECOS_BOARD_HAS_QSPI_BUS
#error "Board ST7735 Display BSP requires a QSPI display resource"
#endif

static ecos_st7735_t display;

ecos_err_t bsp_display_init(void)
{
    const ecos_st7735_config_t config = {
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
    return ecos_st7735_init(&display, &config);
}

ecos_err_t bsp_display_deinit(void)
{
    return ecos_st7735_deinit(&display);
}

ecos_err_t bsp_display_fill_rect(uint16_t x, uint16_t y,
                                 uint16_t width, uint16_t height,
                                 uint16_t rgb565)
{
    uint32_t color = ((uint32_t)rgb565 << 16) | rgb565;
    return ecos_st7735_fill(&display, x, y, width, height, color);
}

uint16_t bsp_display_width(void)
{
    return ECOS_BOARD_DISPLAY_WIDTH;
}

uint16_t bsp_display_height(void)
{
    return ECOS_BOARD_DISPLAY_HEIGHT;
}
