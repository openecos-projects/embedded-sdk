#include "ecos/driver/spi.h"

#include "ecos/driver/gpio.h"
#include "ecos/hal/spi.h"

#include <stddef.h>

static ecos_spi_config_t configs[4];
static uint8_t initialized[4];
static uint8_t selected[4];

static int valid_id(ecos_spi_id_t spi)
{
    return spi < (sizeof(configs) / sizeof(configs[0]));
}

static int valid_mode(ecos_spi_mode_t mode)
{
    return mode >= ECOS_SPI_MODE_0 && mode <= ECOS_SPI_MODE_3;
}

static int valid_order(ecos_spi_bit_order_t order)
{
    return order == ECOS_SPI_MSB_FIRST || order == ECOS_SPI_LSB_FIRST;
}

static ecos_err_t set_cs(const ecos_spi_config_t *config, ecos_gpio_level_t level)
{
    return ecos_gpio_set_level(
        (ecos_gpio_port_t)config->cs_port, config->cs_pin, level
    );
}

int ecos_spi_get_instance_count(void)
{
    return hal_spi_get_instance_count();
}

ecos_err_t ecos_spi_init(ecos_spi_id_t spi, const ecos_spi_config_t *config)
{
    ecos_gpio_config_t gpio = {
        ECOS_GPIO_DIRECTION_OUTPUT, ECOS_GPIO_FUNCTION_GPIO
    };

    if (!valid_id(spi) || config == NULL || config->clock_hz == 0u ||
        !valid_mode(config->mode) || !valid_order(config->bit_order) ||
        config->cs_port >= ECOS_GPIO_PORT_COUNT || config->cs_pin >= 32u)
        return ECOS_ERR_INVALID_ARGUMENT;
    hal_spi_config_t hal_config = {
        config->clock_hz,
        (hal_spi_mode_t)config->mode,
        (hal_spi_bit_order_t)config->bit_order,
    };
    if (hal_spi_init(spi, &hal_config) != ECOS_OK)
        return ECOS_ERR_IO;
    if (ecos_gpio_configure(config->cs_port, config->cs_pin, &gpio) != ECOS_OK)
        return ECOS_ERR_IO;
    if (set_cs(config, ECOS_GPIO_LEVEL_HIGH) != ECOS_OK)
        return ECOS_ERR_IO;
    configs[spi] = *config;
    initialized[spi] = 1u;
    selected[spi] = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_spi_deinit(ecos_spi_id_t spi)
{
    if (!valid_id(spi))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[spi] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    (void)set_cs(&configs[spi], ECOS_GPIO_LEVEL_HIGH);
    selected[spi] = 0u;
    initialized[spi] = 0u;
    return hal_spi_deinit(spi);
}

ecos_err_t ecos_spi_select(ecos_spi_id_t spi)
{
    if (!valid_id(spi))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[spi] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if (selected[spi] != 0u)
        return ECOS_ERR_BUSY;
    selected[spi] = 1u;
    return set_cs(&configs[spi], ECOS_GPIO_LEVEL_LOW);
}

ecos_err_t ecos_spi_deselect(ecos_spi_id_t spi)
{
    if (!valid_id(spi))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[spi] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    selected[spi] = 0u;
    return set_cs(&configs[spi], ECOS_GPIO_LEVEL_HIGH);
}

ecos_err_t ecos_spi_transfer(ecos_spi_id_t spi, const void *tx, void *rx, size_t size)
{
    const uint8_t *tx_bytes = (const uint8_t *)tx;
    uint8_t *rx_bytes = (uint8_t *)rx;
    size_t index;

    if (!valid_id(spi) || (tx == NULL && rx == NULL) || size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[spi] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    for (index = 0u; index < size; ++index) {
        uint8_t received = 0u;
        ecos_err_t result = hal_spi_transfer_byte(
            spi, tx_bytes != NULL ? tx_bytes[index] : 0xFFu, &received
        );
        if (result != ECOS_OK)
            return result;
        if (rx_bytes != NULL)
            rx_bytes[index] = received;
    }
    return ECOS_OK;
}

ecos_err_t ecos_spi_write(ecos_spi_id_t spi, const void *data, size_t size)
{
    return ecos_spi_transfer(spi, data, NULL, size);
}

ecos_err_t ecos_spi_read(ecos_spi_id_t spi, void *data, size_t size)
{
    return ecos_spi_transfer(spi, NULL, data, size);
}
