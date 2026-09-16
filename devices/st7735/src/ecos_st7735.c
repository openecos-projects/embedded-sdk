#include "ecos/device/st7735.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define ST7735_TRY(expression)                 \
    do {                                       \
        ecos_err_t st7735_result = (expression); \
        if (st7735_result != ECOS_OK)           \
            return st7735_result;               \
    } while (0)

static ecos_err_t st7735_command(ecos_st7735_t *display, uint8_t command)
{
    ST7735_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_LOW
    ));
    return ecos_qspi_write_8_cs(
        display->config.qspi, command, display->config.chip_select
    );
}

static ecos_err_t st7735_data8(ecos_st7735_t *display, uint8_t data)
{
    ST7735_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    return ecos_qspi_write_8_cs(
        display->config.qspi, data, display->config.chip_select
    );
}

static ecos_err_t st7735_data16(ecos_st7735_t *display, uint16_t data)
{
    ST7735_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    return ecos_qspi_write_16_cs(
        display->config.qspi, data, display->config.chip_select
    );
}

static ecos_err_t st7735_data32(ecos_st7735_t *display, uint32_t data)
{
    ST7735_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    return ecos_qspi_write_32_cs(
        display->config.qspi, data, display->config.chip_select
    );
}

static ecos_err_t st7735_data32x32(ecos_st7735_t *display, uint32_t data)
{
    ST7735_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    return ecos_qspi_write_32x32_cs(
        display->config.qspi,
        data, data, data, data, data, data, data, data,
        data, data, data, data, data, data, data, data,
        data, data, data, data, data, data, data, data,
        data, data, data, data, data, data, data, data,
        display->config.chip_select
    );
}

static ecos_err_t st7735_init_sequence(ecos_st7735_t *display)
{
    static const uint8_t frame_rate[] = { 0x01u, 0x2Cu, 0x2Du };
    static const uint8_t gamma_positive[] = {
        0x0Fu, 0x1Au, 0x0Fu, 0x18u, 0x2Fu, 0x28u, 0x20u, 0x22u,
        0x1Fu, 0x1Bu, 0x23u, 0x37u, 0x00u, 0x07u, 0x02u, 0x10u,
    };
    static const uint8_t gamma_negative[] = {
        0x0Fu, 0x1Bu, 0x0Fu, 0x17u, 0x33u, 0x2Cu, 0x29u, 0x2Eu,
        0x30u, 0x30u, 0x39u, 0x3Fu, 0x00u, 0x07u, 0x03u, 0x10u,
    };
    size_t index;
    uint8_t madctl;

    ST7735_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 120u));
    ST7735_TRY(st7735_command(display, 0x11u));
    ST7735_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 120u));

    ST7735_TRY(st7735_command(display, 0xB1u));
    for (index = 0; index < sizeof(frame_rate); ++index)
        ST7735_TRY(st7735_data8(display, frame_rate[index]));
    ST7735_TRY(st7735_command(display, 0xB2u));
    for (index = 0; index < sizeof(frame_rate); ++index)
        ST7735_TRY(st7735_data8(display, frame_rate[index]));
    ST7735_TRY(st7735_command(display, 0xB3u));
    for (index = 0; index < sizeof(frame_rate); ++index)
        ST7735_TRY(st7735_data8(display, frame_rate[index]));
    for (index = 0; index < sizeof(frame_rate); ++index)
        ST7735_TRY(st7735_data8(display, frame_rate[index]));
    ST7735_TRY(st7735_command(display, 0xB4u));
    ST7735_TRY(st7735_data8(display, 0x07u));

    ST7735_TRY(st7735_command(display, 0xC0u));
    ST7735_TRY(st7735_data8(display, 0xA2u));
    ST7735_TRY(st7735_data8(display, 0x02u));
    ST7735_TRY(st7735_data8(display, 0x84u));
    ST7735_TRY(st7735_command(display, 0xC1u));
    ST7735_TRY(st7735_data8(display, 0xC5u));
    ST7735_TRY(st7735_command(display, 0xC2u));
    ST7735_TRY(st7735_data8(display, 0x0Au));
    ST7735_TRY(st7735_data8(display, 0x00u));
    ST7735_TRY(st7735_command(display, 0xC3u));
    ST7735_TRY(st7735_data8(display, 0x8Au));
    ST7735_TRY(st7735_data8(display, 0x2Au));
    ST7735_TRY(st7735_command(display, 0xC4u));
    ST7735_TRY(st7735_data8(display, 0x8Au));
    ST7735_TRY(st7735_data8(display, 0xEEu));
    ST7735_TRY(st7735_command(display, 0xC5u));
    ST7735_TRY(st7735_data8(display, 0x0Eu));

    ST7735_TRY(st7735_command(display, 0x36u));
    switch (display->config.rotation) {
    case 0u:
        madctl = 0xC8u;
        break;
    case 1u:
        madctl = 0xA8u;
        break;
    case 2u:
        madctl = 0x08u;
        break;
    default:
        madctl = 0x68u;
        break;
    }
    ST7735_TRY(st7735_data8(display, madctl));

    ST7735_TRY(st7735_command(display, 0xE0u));
    for (index = 0; index < sizeof(gamma_positive); ++index)
        ST7735_TRY(st7735_data8(display, gamma_positive[index]));
    ST7735_TRY(st7735_command(display, 0xE1u));
    for (index = 0; index < sizeof(gamma_negative); ++index)
        ST7735_TRY(st7735_data8(display, gamma_negative[index]));

    ST7735_TRY(st7735_command(display, 0xF0u));
    ST7735_TRY(st7735_data8(display, 0x01u));
    ST7735_TRY(st7735_command(display, 0xF6u));
    ST7735_TRY(st7735_data8(display, 0x00u));
    ST7735_TRY(st7735_command(display, 0x3Au));
    ST7735_TRY(st7735_data8(display, 0x05u));
    ST7735_TRY(st7735_command(display, 0x29u));
    return ECOS_OK;
}

ecos_err_t ecos_st7735_init(ecos_st7735_t *display,
                            const ecos_st7735_config_t *config)
{
    ecos_gpio_config_t gpio_config = {
        ECOS_GPIO_DIRECTION_OUTPUT,
        ECOS_GPIO_FUNCTION_GPIO,
    };
    ecos_qspi_config_t qspi_config;

    if (display == NULL || config == NULL || config->width == 0u ||
        config->height == 0u || config->rotation > 3u)
        return ECOS_ERR_INVALID_ARGUMENT;
    qspi_config.clock_divider = config->qspi_clock_divider;
    ST7735_TRY(ecos_qspi_init(config->qspi, &qspi_config));
    ST7735_TRY(ecos_gpio_configure(
        config->dc_port, config->dc_pin, &gpio_config
    ));
    ST7735_TRY(ecos_gpio_configure(
        config->reset_port, config->reset_pin, &gpio_config
    ));
    ST7735_TRY(ecos_gpio_configure(
        config->backlight_port, config->backlight_pin, &gpio_config
    ));
    /* The board owns the panel reset and backlight policy: both stay high. */
    ST7735_TRY(ecos_gpio_set_level(
        config->reset_port, config->reset_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    ST7735_TRY(ecos_gpio_set_level(
        config->backlight_port, config->backlight_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    ST7735_TRY(ecos_gpio_set_level(
        config->dc_port, config->dc_pin, ECOS_GPIO_LEVEL_LOW
    ));
    display->config = *config;
    display->initialized = 0u;
    ST7735_TRY(st7735_init_sequence(display));
    display->initialized = 1u;
    return ECOS_OK;
}

ecos_err_t ecos_st7735_deinit(ecos_st7735_t *display)
{
    if (display == NULL || display->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    display->initialized = 0u;
    return ecos_qspi_deinit(display->config.qspi);
}

ecos_err_t ecos_st7735_set_window(ecos_st7735_t *display,
                                  uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  uint16_t height)
{
    uint32_t x_end_value;
    uint32_t y_end_value;
    uint16_t x_end;
    uint16_t y_end;

    if (display == NULL || display->initialized == 0u || width == 0u ||
        height == 0u || x >= display->config.width || y >= display->config.height)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (width > display->config.width - x || height > display->config.height - y)
        return ECOS_ERR_INVALID_ARGUMENT;
    x_end_value = (uint32_t)x + width - 1u + display->config.horizontal_offset;
    y_end_value = (uint32_t)y + height - 1u + display->config.vertical_offset;
    if (x_end_value > UINT16_MAX || y_end_value > UINT16_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    x_end = (uint16_t)x_end_value;
    y_end = (uint16_t)y_end_value;
    ST7735_TRY(st7735_command(display, 0x2Au));
    ST7735_TRY(st7735_data16(
        display, (uint16_t)(x + display->config.horizontal_offset)
    ));
    ST7735_TRY(st7735_data16(display, x_end));
    ST7735_TRY(st7735_command(display, 0x2Bu));
    ST7735_TRY(st7735_data16(
        display, (uint16_t)(y + display->config.vertical_offset)
    ));
    ST7735_TRY(st7735_data16(display, y_end));
    return st7735_command(display, 0x2Cu);
}

ecos_err_t ecos_st7735_fill(ecos_st7735_t *display,
                            uint16_t x,
                            uint16_t y,
                            uint16_t width,
                            uint16_t height,
                            uint32_t color)
{
    uint32_t row;
    uint32_t pixels;
    ecos_err_t result;

    if (display == NULL || display->initialized == 0u || width == 0u ||
        height == 0u || x >= display->config.width || y >= display->config.height ||
        width > display->config.width - x || height > display->config.height - y)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = ecos_st7735_set_window(display, x, y, width, height);
    if (result != ECOS_OK)
        return result;
    /* Keep the freestanding rv32e image independent of libgcc's __mulsi3. */
    for (row = 0u; row < height; ++row) {
        pixels = width;
        while (pixels >= 64u) {
            result = st7735_data32x32(display, color);
            if (result != ECOS_OK)
                return result;
            pixels -= 64u;
        }
        while (pixels >= 2u) {
            result = st7735_data32(display, color);
            if (result != ECOS_OK)
                return result;
            pixels -= 2u;
        }
        if (pixels != 0u) {
            result = st7735_data16(display, (uint16_t)(color >> 16));
            if (result != ECOS_OK)
                return result;
        }
    }
    return ECOS_OK;
}
