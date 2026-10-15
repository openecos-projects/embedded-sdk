#include "ecos/soft_spi.h"

#include <string.h>

static int pin_is_valid(ecos_gpio_pin_t pin)
{
    return pin.port < ECOS_GPIO_PORT_COUNT && pin.pin < 32u;
}

static int mode_is_valid(ecos_soft_spi_mode_t mode)
{
    return mode >= ECOS_SOFT_SPI_MODE_0 && mode <= ECOS_SOFT_SPI_MODE_3;
}

static int mode_is_cpol_high(ecos_soft_spi_mode_t mode)
{
    return mode >= ECOS_SOFT_SPI_MODE_2;
}

static int mode_is_cpha_one(ecos_soft_spi_mode_t mode)
{
    return (mode & 1u) != 0u;
}

static void delay(const ecos_soft_spi_t *bus)
{
    if (bus->config.delay != NULL)
        bus->config.delay(bus->config.delay_context);
}

static ecos_err_t set_level(ecos_gpio_pin_t pin, int level)
{
    return ecos_gpio_set_level(
        pin.port, pin.pin,
        level ? ECOS_GPIO_LEVEL_HIGH : ECOS_GPIO_LEVEL_LOW
    );
}

static ecos_err_t configure_pin(ecos_gpio_pin_t pin,
                                ecos_gpio_direction_t direction)
{
    const ecos_gpio_config_t config = {
        .direction = direction,
        .function = ECOS_GPIO_FUNCTION_GPIO,
    };
    return ecos_gpio_configure(pin.port, pin.pin, &config);
}

static ecos_err_t validate_config(const ecos_soft_spi_config_t *config)
{
    if (config == NULL || !pin_is_valid(config->chip_select) ||
        !pin_is_valid(config->clock) || !pin_is_valid(config->mosi) ||
        !pin_is_valid(config->miso) || !mode_is_valid(config->mode))
        return ECOS_ERR_INVALID_ARGUMENT;
    if ((config->chip_select.port == config->clock.port &&
         config->chip_select.pin == config->clock.pin) ||
        (config->chip_select.port == config->mosi.port &&
         config->chip_select.pin == config->mosi.pin) ||
        (config->chip_select.port == config->miso.port &&
         config->chip_select.pin == config->miso.pin) ||
        (config->clock.port == config->mosi.port &&
         config->clock.pin == config->mosi.pin) ||
        (config->clock.port == config->miso.port &&
         config->clock.pin == config->miso.pin) ||
        (config->mosi.port == config->miso.port &&
         config->mosi.pin == config->miso.pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_OK;
}

ecos_err_t ecos_soft_spi_init(ecos_soft_spi_t *bus,
                              const ecos_soft_spi_config_t *config)
{
    ecos_err_t result;
    int idle;

    if (bus == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (bus->active)
        return ECOS_ERR_BUSY;
    result = validate_config(config);
    if (result != ECOS_OK)
        return result;
    bus->initialized = false;
    bus->active = false;

    idle = mode_is_cpol_high(config->mode);
    result = set_level(config->chip_select, 1);
    if (result != ECOS_OK)
        return result;
    result = set_level(config->clock, idle);
    if (result != ECOS_OK)
        return result;
    result = set_level(config->mosi, 0);
    if (result != ECOS_OK)
        return result;

    result = configure_pin(config->miso, ECOS_GPIO_DIRECTION_INPUT);
    if (result != ECOS_OK)
        return result;
    result = configure_pin(config->chip_select, ECOS_GPIO_DIRECTION_OUTPUT);
    if (result != ECOS_OK)
        return result;
    result = configure_pin(config->clock, ECOS_GPIO_DIRECTION_OUTPUT);
    if (result != ECOS_OK)
        return result;
    result = configure_pin(config->mosi, ECOS_GPIO_DIRECTION_OUTPUT);
    if (result != ECOS_OK)
        return result;
    memcpy(&bus->config, config, sizeof(bus->config));
    bus->initialized = true;
    return ECOS_OK;
}

ecos_err_t ecos_soft_spi_start(ecos_soft_spi_t *bus)
{
    ecos_err_t result;
    if (bus == NULL || !bus->initialized)
        return ECOS_ERR_NOT_INITIALIZED;
    if (bus->active)
        return ECOS_ERR_BUSY;
    result = set_level(bus->config.chip_select, 0);
    if (result == ECOS_OK)
        bus->active = true;
    return result;
}

ecos_err_t ecos_soft_spi_stop(ecos_soft_spi_t *bus)
{
    ecos_err_t result;
    if (bus == NULL || !bus->initialized)
        return ECOS_ERR_NOT_INITIALIZED;
    if (!bus->active)
        return ECOS_ERR_INVALID_STATE;
    result = set_level(bus->config.chip_select, 1);
    if (result == ECOS_OK)
        bus->active = false;
    return result;
}

ecos_err_t ecos_soft_spi_transfer_byte(ecos_soft_spi_t *bus,
                                       uint8_t tx,
                                       uint8_t *rx)
{
    int cpol;
    int cpha;
    int leading;
    int trailing;
    ecos_err_t result;
    uint8_t received = 0;

    if (bus == NULL || !bus->initialized)
        return ECOS_ERR_NOT_INITIALIZED;
    if (!bus->active)
        return ECOS_ERR_INVALID_STATE;
    if (rx == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    cpol = mode_is_cpol_high(bus->config.mode);
    cpha = mode_is_cpha_one(bus->config.mode);
    leading = !cpol;
    trailing = cpol;
    for (unsigned int bit = 0; bit < 8u; ++bit) {
        if (!cpha) {
            result = set_level(bus->config.mosi, (tx & 0x80u) != 0u);
            if (result != ECOS_OK)
                return result;
            delay(bus);
            result = set_level(bus->config.clock, leading);
            if (result != ECOS_OK)
                return result;
            delay(bus);
            {
                int level = ecos_gpio_get_level(bus->config.miso.port,
                                                bus->config.miso.pin);
                if (level < 0)
                    return (ecos_err_t)level;
                received = (uint8_t)((received << 1) | (level != 0));
            }
            result = set_level(bus->config.clock, trailing);
            if (result != ECOS_OK)
                return result;
            delay(bus);
        } else {
            result = set_level(bus->config.clock, leading);
            if (result != ECOS_OK)
                return result;
            delay(bus);
            result = set_level(bus->config.mosi, (tx & 0x80u) != 0u);
            if (result != ECOS_OK)
                return result;
            delay(bus);
            result = set_level(bus->config.clock, trailing);
            if (result != ECOS_OK)
                return result;
            delay(bus);
            {
                int level = ecos_gpio_get_level(bus->config.miso.port,
                                                bus->config.miso.pin);
                if (level < 0)
                    return (ecos_err_t)level;
                received = (uint8_t)((received << 1) | (level != 0));
            }
        }
        tx <<= 1;
    }
    *rx = received;
    return ECOS_OK;
}
