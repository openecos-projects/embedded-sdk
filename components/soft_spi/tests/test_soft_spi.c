#include "ecos/soft_spi.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static uint8_t levels[4][32];
static unsigned int events;
static unsigned int delays;
static uint8_t miso_bits;
static unsigned int miso_index;
static uint8_t transmitted_bits;
static ecos_soft_spi_mode_t current_mode;

ecos_err_t ecos_gpio_configure(ecos_gpio_port_t port, uint8_t pin,
                               const ecos_gpio_config_t *config)
{
    assert(port < ECOS_GPIO_PORT_COUNT);
    assert(pin < 32u);
    assert(config != NULL);
    if (pin == 0u && config->direction == ECOS_GPIO_DIRECTION_OUTPUT)
        assert(levels[port][pin] == ECOS_GPIO_LEVEL_HIGH);
    return ECOS_OK;
}

ecos_err_t ecos_gpio_set_level(ecos_gpio_port_t port, uint8_t pin,
                               ecos_gpio_level_t level)
{
    assert(port < ECOS_GPIO_PORT_COUNT);
    assert(pin < 32u);
    levels[port][pin] = (uint8_t)level;
    ++events;
    return ECOS_OK;
}

int ecos_gpio_get_level(ecos_gpio_port_t port, uint8_t pin)
{
    int expected_clock = (current_mode & 1u) != 0u ?
        current_mode >= ECOS_SOFT_SPI_MODE_2 :
        current_mode < ECOS_SOFT_SPI_MODE_2;
    assert(port == ECOS_GPIO_PORT_0);
    assert(pin == 3u);
    assert(miso_index < 8u);
    assert(levels[0][0] == ECOS_GPIO_LEVEL_LOW);
    assert(levels[0][1] == expected_clock);
    assert(levels[0][2] == ((transmitted_bits >> (7u - miso_index)) & 1u));
    return (miso_bits >> (7u - miso_index++)) & 1u;
}

static void count_delay(void *context)
{
    (void)context;
    ++delays;
}

static ecos_soft_spi_config_t config(ecos_soft_spi_mode_t mode)
{
    const ecos_soft_spi_config_t value = {
        .chip_select = { ECOS_GPIO_PORT_0, 0u },
        .clock = { ECOS_GPIO_PORT_0, 1u },
        .mosi = { ECOS_GPIO_PORT_0, 2u },
        .miso = { ECOS_GPIO_PORT_0, 3u },
        .mode = mode,
        .delay = count_delay,
        .delay_context = NULL,
    };
    return value;
}

static void check_transfer(ecos_soft_spi_mode_t mode)
{
    ecos_soft_spi_t bus = {0};
    uint8_t received = 0;
    ecos_soft_spi_config_t value = config(mode);

    events = 0;
    delays = 0;
    miso_bits = 0xA5u;
    miso_index = 0;
    transmitted_bits = 0x3Cu;
    current_mode = mode;
    assert(ecos_soft_spi_init(&bus, &value) == ECOS_OK);
    assert(ecos_soft_spi_stop(&bus) == ECOS_ERR_INVALID_STATE);
    assert(ecos_soft_spi_transfer_byte(&bus, transmitted_bits, &received) ==
           ECOS_ERR_INVALID_STATE);
    assert(ecos_soft_spi_start(&bus) == ECOS_OK);
    assert(ecos_soft_spi_start(&bus) == ECOS_ERR_BUSY);
    assert(ecos_soft_spi_init(&bus, &value) == ECOS_ERR_BUSY);
    assert(ecos_soft_spi_transfer_byte(&bus, transmitted_bits, &received) == ECOS_OK);
    assert(received == 0xA5u);
    assert(miso_index == 8u);
    assert(ecos_soft_spi_stop(&bus) == ECOS_OK);
    assert(ecos_soft_spi_stop(&bus) == ECOS_ERR_INVALID_STATE);
    assert(levels[0][0] == ECOS_GPIO_LEVEL_HIGH);
    assert(levels[0][1] == (mode >= ECOS_SOFT_SPI_MODE_2));
    assert(events == 29u);
    assert(delays == 24u);
}

int main(void)
{
    ecos_soft_spi_t bus = {0};
    ecos_soft_spi_config_t invalid = config(ECOS_SOFT_SPI_MODE_0);
    invalid.miso = invalid.clock;
    assert(ecos_soft_spi_init(&bus, &invalid) == ECOS_ERR_INVALID_ARGUMENT);
    invalid = config((ecos_soft_spi_mode_t)-1);
    assert(ecos_soft_spi_init(&bus, &invalid) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_soft_spi_start(NULL) == ECOS_ERR_NOT_INITIALIZED);
    check_transfer(ECOS_SOFT_SPI_MODE_0);
    check_transfer(ECOS_SOFT_SPI_MODE_1);
    check_transfer(ECOS_SOFT_SPI_MODE_2);
    check_transfer(ECOS_SOFT_SPI_MODE_3);
    return 0;
}
