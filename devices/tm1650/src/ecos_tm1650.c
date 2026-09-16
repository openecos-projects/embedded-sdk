#include "ecos/device/tm1650.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define TM1650_TRY(expression)                   \
    do {                                         \
        ecos_err_t tm1650_result = (expression); \
        if (tm1650_result != ECOS_OK)            \
            return tm1650_result;                \
    } while (0)

#define TM1650_CMD_SYSTEM 0x48u
#define TM1650_DIGIT_ADDRESS_BASE 0x68u /* +2 per position */
#define TM1650_MODE_8SEG 0x01u
#define TM1650_HALF_PERIOD_US 3u

/* Common-cathode font, bit0=A .. bit6=G. */
static const uint8_t tm1650_font[16] = {
    0x3Fu, 0x06u, 0x5Bu, 0x4Fu, /* 0 1 2 3 */
    0x66u, 0x6Du, 0x7Du, 0x07u, /* 4 5 6 7 */
    0x7Fu, 0x6Fu, 0x77u, 0x7Cu, /* 8 9 A b */
    0x39u, 0x5Eu, 0x79u, 0x71u, /* C d E F */
};

/* Open-drain emulation: the pins have external pull-ups, so "high" is the
 * input direction and "low" is output (level was set low at init). */
static ecos_err_t tm1650_line_low(ecos_gpio_port_t port, uint8_t pin)
{
    return ecos_gpio_set_direction(port, pin, ECOS_GPIO_DIRECTION_OUTPUT);
}

static ecos_err_t tm1650_line_high(ecos_gpio_port_t port, uint8_t pin)
{
    return ecos_gpio_set_direction(port, pin, ECOS_GPIO_DIRECTION_INPUT);
}

static ecos_err_t tm1650_delay(void)
{
    return ecos_timer_delay_us(ECOS_TIMER_DEFAULT, TM1650_HALF_PERIOD_US);
}

static ecos_err_t tm1650_start(const ecos_tm1650_t *display)
{
    /* Lines are already released (high) in the idle state. */
    TM1650_TRY(tm1650_delay());
    TM1650_TRY(tm1650_line_low(display->config.dat_port,
                               display->config.dat_pin));
    TM1650_TRY(tm1650_delay());
    TM1650_TRY(tm1650_line_low(display->config.clk_port,
                               display->config.clk_pin));
    return tm1650_delay();
}

static ecos_err_t tm1650_stop(const ecos_tm1650_t *display)
{
    TM1650_TRY(tm1650_line_low(display->config.dat_port,
                               display->config.dat_pin));
    TM1650_TRY(tm1650_delay());
    TM1650_TRY(tm1650_line_high(display->config.clk_port,
                                display->config.clk_pin));
    TM1650_TRY(tm1650_delay());
    TM1650_TRY(tm1650_line_high(display->config.dat_port,
                                display->config.dat_pin));
    return tm1650_delay();
}

static ecos_err_t tm1650_write_byte(const ecos_tm1650_t *display, uint8_t byte)
{
    uint8_t bit;
    int ack_level;

    for (bit = 0u; bit < 8u; ++bit) {
        TM1650_TRY(tm1650_line_low(display->config.clk_port,
                                   display->config.clk_pin));
        if ((byte & 0x80u) != 0u) {
            TM1650_TRY(tm1650_line_high(display->config.dat_port,
                                        display->config.dat_pin));
        } else {
            TM1650_TRY(tm1650_line_low(display->config.dat_port,
                                       display->config.dat_pin));
        }
        TM1650_TRY(tm1650_delay());
        TM1650_TRY(tm1650_line_high(display->config.clk_port,
                                    display->config.clk_pin));
        TM1650_TRY(tm1650_delay());
        byte = (uint8_t)(byte << 1);
    }

    /* Ninth clock: release DAT and expect the device to pull it low. */
    TM1650_TRY(tm1650_line_low(display->config.clk_port,
                               display->config.clk_pin));
    TM1650_TRY(tm1650_line_high(display->config.dat_port,
                                display->config.dat_pin));
    TM1650_TRY(tm1650_delay());
    TM1650_TRY(tm1650_line_high(display->config.clk_port,
                                display->config.clk_pin));
    TM1650_TRY(tm1650_delay());
    ack_level = ecos_gpio_get_level(display->config.dat_port,
                                    display->config.dat_pin);
    if (ack_level < 0)
        return (ecos_err_t)ack_level;
    TM1650_TRY(tm1650_line_low(display->config.clk_port,
                               display->config.clk_pin));
    if (ack_level != (int)ECOS_GPIO_LEVEL_LOW)
        return ECOS_ERR_IO;
    return ECOS_OK;
}

static ecos_err_t tm1650_write_command(const ecos_tm1650_t *display,
                                       uint8_t address,
                                       uint8_t data)
{
    TM1650_TRY(tm1650_start(display));
    TM1650_TRY(tm1650_write_byte(display, address));
    TM1650_TRY(tm1650_write_byte(display, data));
    return tm1650_stop(display);
}

static ecos_err_t tm1650_check_ready(const ecos_tm1650_t *display,
                                     uint8_t position)
{
    if (display == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (display->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if (position >= ECOS_TM1650_POSITION_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_OK;
}

ecos_err_t ecos_tm1650_init(ecos_tm1650_t *display,
                            const ecos_tm1650_config_t *config)
{
    static const ecos_gpio_config_t output = {
        ECOS_GPIO_DIRECTION_OUTPUT, ECOS_GPIO_FUNCTION_GPIO
    };

    if (display == NULL || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    display->config = *config;
    display->brightness = 0u;
    display->initialized = 0u;

    TM1650_TRY(ecos_gpio_configure(config->clk_port, config->clk_pin,
                                   &output));
    TM1650_TRY(ecos_gpio_configure(config->dat_port, config->dat_pin,
                                   &output));
    TM1650_TRY(ecos_gpio_set_level(config->clk_port, config->clk_pin,
                                   ECOS_GPIO_LEVEL_LOW));
    TM1650_TRY(ecos_gpio_set_level(config->dat_port, config->dat_pin,
                                   ECOS_GPIO_LEVEL_LOW));
    TM1650_TRY(tm1650_line_high(config->clk_port, config->clk_pin));
    TM1650_TRY(tm1650_line_high(config->dat_port, config->dat_pin));

    display->initialized = 1u;
    TM1650_TRY(ecos_tm1650_clear(display));
    TM1650_TRY(ecos_tm1650_set_brightness(display,
                                          ECOS_TM1650_BRIGHTNESS_DEFAULT));
    return ECOS_OK;
}

ecos_err_t ecos_tm1650_deinit(ecos_tm1650_t *display)
{
    if (display == NULL || display->initialized == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    TM1650_TRY(ecos_tm1650_set_brightness(display,
                                          ECOS_TM1650_BRIGHTNESS_OFF));
    display->initialized = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_tm1650_set_brightness(ecos_tm1650_t *display, uint8_t level)
{
    uint8_t data;

    if (display == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (display->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if (level > ECOS_TM1650_BRIGHTNESS_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;

    data = level == 0u
               ? 0x00u
               : (uint8_t)(((uint8_t)(level - 1u) << 4) | TM1650_MODE_8SEG);
    TM1650_TRY(tm1650_write_command(display, TM1650_CMD_SYSTEM, data));
    display->brightness = level;
    return ECOS_OK;
}

ecos_err_t ecos_tm1650_clear(ecos_tm1650_t *display)
{
    uint8_t position;

    if (display == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (display->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    for (position = 0u; position < ECOS_TM1650_POSITION_COUNT; ++position)
        TM1650_TRY(ecos_tm1650_set_segments(display, position, 0u));
    return ECOS_OK;
}

ecos_err_t ecos_tm1650_set_segments(ecos_tm1650_t *display,
                                    uint8_t position,
                                    uint8_t segments)
{
    TM1650_TRY(tm1650_check_ready(display, position));
    return tm1650_write_command(
        display,
        (uint8_t)(TM1650_DIGIT_ADDRESS_BASE + (uint8_t)(position * 2u)),
        segments
    );
}

ecos_err_t ecos_tm1650_show_digit(ecos_tm1650_t *display,
                                  uint8_t position,
                                  uint8_t value,
                                  bool dot)
{
    uint8_t segments;

    TM1650_TRY(tm1650_check_ready(display, position));
    if (value > 15u)
        return ECOS_ERR_INVALID_ARGUMENT;

    segments = tm1650_font[value];
    if (dot)
        segments |= ECOS_TM1650_SEGMENT_DOT;
    return ecos_tm1650_set_segments(display, position, segments);
}

ecos_err_t ecos_tm1650_show_number(ecos_tm1650_t *display,
                                   uint16_t value,
                                   bool leading_zero)
{
    uint8_t digits[ECOS_TM1650_POSITION_COUNT];
    static const uint16_t weights[ECOS_TM1650_POSITION_COUNT - 1u] = {
        1000u, 100u, 10u
    };
    uint8_t position;
    uint8_t started;
    ecos_err_t result;

    if (display == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (display->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if (value > 9999u)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* The rv32e runtime links without libgcc division helpers, so extract
     * the decimal digits by repeated subtraction instead of / and %. */
    for (position = 0u; position < ECOS_TM1650_POSITION_COUNT; ++position) {
        uint8_t digit = 0u;

        if (position < ECOS_TM1650_POSITION_COUNT - 1u) {
            uint16_t weight = weights[position];

            while (value >= weight) {
                value = (uint16_t)(value - weight);
                ++digit;
            }
        } else {
            digit = (uint8_t)value;
        }
        digits[position] = digit;
    }

    started = leading_zero ? 1u : 0u;
    for (position = 0u; position < ECOS_TM1650_POSITION_COUNT; ++position) {
        uint8_t segments;

        if (digits[position] != 0u)
            started = 1u;
        if (started != 0u || position == ECOS_TM1650_POSITION_COUNT - 1u)
            segments = tm1650_font[digits[position]];
        else
            segments = 0u;

        result = ecos_tm1650_set_segments(display, position, segments);
        if (result != ECOS_OK)
            return result;
    }
    return ECOS_OK;
}
