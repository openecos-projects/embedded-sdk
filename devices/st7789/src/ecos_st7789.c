#include "ecos/device/st7789.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define ST7789_TRY(expression)                 \
    do {                                       \
        ecos_err_t st7789_result = (expression); \
        if (st7789_result != ECOS_OK)           \
            return st7789_result;               \
    } while (0)

static ecos_err_t st7789_command(ecos_st7789_t *display, uint8_t command)
{
    ST7789_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_LOW
    ));
    return ecos_qspi_write_8_cs(
        display->config.qspi, command, display->config.chip_select
    );
}

static ecos_err_t st7789_data8(ecos_st7789_t *display, uint8_t data)
{
    ST7789_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    return ecos_qspi_write_8_cs(
        display->config.qspi, data, display->config.chip_select
    );
}

static ecos_err_t st7789_data16(ecos_st7789_t *display, uint16_t data)
{
    ST7789_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    return ecos_qspi_write_16_cs(
        display->config.qspi, data, display->config.chip_select
    );
}

static ecos_err_t st7789_data32(ecos_st7789_t *display, uint32_t data)
{
    ST7789_TRY(ecos_gpio_set_level(
        display->config.dc_port, display->config.dc_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    return ecos_qspi_write_32_cs(
        display->config.qspi, data, display->config.chip_select
    );
}

static ecos_err_t st7789_data32x32(ecos_st7789_t *display, uint32_t data)
{
    ST7789_TRY(ecos_gpio_set_level(
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

static ecos_err_t st7789_init_sequence(ecos_st7789_t *display)
{
    static const uint8_t porch_control[] = {
        0x0Cu, 0x0Cu, 0x00u, 0x33u, 0x33u,
    };
    static const uint8_t gamma_positive[] = {
        0xD0u, 0x04u, 0x0Du, 0x11u, 0x13u, 0x2Bu, 0x3Fu,
        0x54u, 0x4Cu, 0x18u, 0x0Du, 0x0Bu, 0x1Fu, 0x23u,
    };
    static const uint8_t gamma_negative[] = {
        0xD0u, 0x04u, 0x0Cu, 0x11u, 0x13u, 0x2Cu, 0x3Fu,
        0x44u, 0x51u, 0x2Fu, 0x1Fu, 0x1Fu, 0x20u, 0x23u,
    };
    size_t index;
    uint8_t madctl;

    ST7789_TRY(ecos_gpio_set_level(
        display->config.reset_port, display->config.reset_pin,
        ECOS_GPIO_LEVEL_LOW
    ));
    ST7789_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 100u));
    ST7789_TRY(ecos_gpio_set_level(
        display->config.reset_port, display->config.reset_pin,
        ECOS_GPIO_LEVEL_HIGH
    ));
    ST7789_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 100u));

    ST7789_TRY(st7789_command(display, 0x11u));
    ST7789_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 120u));

    ST7789_TRY(st7789_command(display, 0x36u));
    switch (display->config.rotation) {
    case 0u:
        madctl = 0x00u;
        break;
    case 1u:
        madctl = 0xC0u;
        break;
    case 2u:
        madctl = 0x70u;
        break;
    default:
        madctl = 0xA0u;
        break;
    }
    ST7789_TRY(st7789_data8(display, madctl));

    ST7789_TRY(st7789_command(display, 0x3Au));
    ST7789_TRY(st7789_data8(display, 0x05u));

    ST7789_TRY(st7789_command(display, 0xB2u));
    for (index = 0; index < sizeof(porch_control); ++index)
        ST7789_TRY(st7789_data8(display, porch_control[index]));
    ST7789_TRY(st7789_command(display, 0xB7u));
    ST7789_TRY(st7789_data8(display, 0x35u));
    ST7789_TRY(st7789_command(display, 0xBBu));
    ST7789_TRY(st7789_data8(display, 0x19u));
    ST7789_TRY(st7789_command(display, 0xC0u));
    ST7789_TRY(st7789_data8(display, 0x2Cu));
    ST7789_TRY(st7789_command(display, 0xC2u));
    ST7789_TRY(st7789_data8(display, 0x01u));
    ST7789_TRY(st7789_command(display, 0xC3u));
    ST7789_TRY(st7789_data8(display, 0x12u));
    ST7789_TRY(st7789_command(display, 0xC4u));
    ST7789_TRY(st7789_data8(display, 0x20u));
    ST7789_TRY(st7789_command(display, 0xC6u));
    ST7789_TRY(st7789_data8(display, 0x0Fu));
    ST7789_TRY(st7789_command(display, 0xD0u));
    ST7789_TRY(st7789_data8(display, 0xA4u));
    ST7789_TRY(st7789_data8(display, 0xA1u));

    ST7789_TRY(st7789_command(display, 0xE0u));
    for (index = 0; index < sizeof(gamma_positive); ++index)
        ST7789_TRY(st7789_data8(display, gamma_positive[index]));
    ST7789_TRY(st7789_command(display, 0xE1u));
    for (index = 0; index < sizeof(gamma_negative); ++index)
        ST7789_TRY(st7789_data8(display, gamma_negative[index]));

    ST7789_TRY(st7789_command(display, 0x21u));
    ST7789_TRY(st7789_command(display, 0x29u));
    return ECOS_OK;
}

ecos_err_t ecos_st7789_init(ecos_st7789_t *display,
                            const ecos_st7789_config_t *config)
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
    ST7789_TRY(ecos_qspi_init(config->qspi, &qspi_config));
    ST7789_TRY(ecos_gpio_configure(
        config->dc_port, config->dc_pin, &gpio_config
    ));
    ST7789_TRY(ecos_gpio_configure(
        config->reset_port, config->reset_pin, &gpio_config
    ));
    ST7789_TRY(ecos_gpio_configure(
        config->backlight_port, config->backlight_pin, &gpio_config
    ));
    ST7789_TRY(ecos_gpio_set_level(
        config->backlight_port, config->backlight_pin, ECOS_GPIO_LEVEL_HIGH
    ));
    ST7789_TRY(ecos_gpio_set_level(
        config->dc_port, config->dc_pin, ECOS_GPIO_LEVEL_LOW
    ));
    display->config = *config;
    display->initialized = 0u;
    ST7789_TRY(st7789_init_sequence(display));
    display->initialized = 1u;
    return ECOS_OK;
}

ecos_err_t ecos_st7789_deinit(ecos_st7789_t *display)
{
    if (display == NULL || display->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    display->initialized = 0u;
    return ecos_qspi_deinit(display->config.qspi);
}

ecos_err_t ecos_st7789_set_window(ecos_st7789_t *display,
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
    ST7789_TRY(st7789_command(display, 0x2Au));
    ST7789_TRY(st7789_data16(
        display, (uint16_t)(x + display->config.horizontal_offset)
    ));
    ST7789_TRY(st7789_data16(display, x_end));
    ST7789_TRY(st7789_command(display, 0x2Bu));
    ST7789_TRY(st7789_data16(
        display, (uint16_t)(y + display->config.vertical_offset)
    ));
    ST7789_TRY(st7789_data16(display, y_end));
    return st7789_command(display, 0x2Cu);
}

ecos_err_t ecos_st7789_fill(ecos_st7789_t *display,
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
    result = ecos_st7789_set_window(display, x, y, width, height);
    if (result != ECOS_OK)
        return result;
    /* Keep the freestanding rv32e image independent of libgcc's __mulsi3. */
    for (row = 0u; row < height; ++row) {
        pixels = width;
        while (pixels >= 64u) {
            result = st7789_data32x32(display, color);
            if (result != ECOS_OK)
                return result;
            pixels -= 64u;
        }
        while (pixels >= 2u) {
            result = st7789_data32(display, color);
            if (result != ECOS_OK)
                return result;
            pixels -= 2u;
        }
        if (pixels != 0u) {
            result = st7789_data16(display, (uint16_t)(color >> 16));
            if (result != ECOS_OK)
                return result;
        }
    }
    return ECOS_OK;
}
