#ifndef ECOS_SOFT_SPI_H
#define ECOS_SOFT_SPI_H

#include "ecos/driver/gpio.h"
#include "ecos/error.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_SOFT_SPI_MODE_0 = 0,
    ECOS_SOFT_SPI_MODE_1,
    ECOS_SOFT_SPI_MODE_2,
    ECOS_SOFT_SPI_MODE_3,
} ecos_soft_spi_mode_t;

typedef void (*ecos_soft_spi_delay_fn)(void *context);

typedef struct {
    ecos_gpio_pin_t chip_select;
    ecos_gpio_pin_t clock;
    ecos_gpio_pin_t mosi;
    ecos_gpio_pin_t miso;
    ecos_soft_spi_mode_t mode;
    ecos_soft_spi_delay_fn delay;
    void *delay_context;
} ecos_soft_spi_config_t;

typedef struct {
    ecos_soft_spi_config_t config;
    bool initialized;
    bool active;
} ecos_soft_spi_t;

ecos_err_t ecos_soft_spi_init(ecos_soft_spi_t *bus,
                              const ecos_soft_spi_config_t *config);
ecos_err_t ecos_soft_spi_start(ecos_soft_spi_t *bus);
ecos_err_t ecos_soft_spi_stop(ecos_soft_spi_t *bus);
ecos_err_t ecos_soft_spi_transfer_byte(ecos_soft_spi_t *bus,
                                       uint8_t tx,
                                       uint8_t *rx);

#ifdef __cplusplus
}
#endif

#endif
