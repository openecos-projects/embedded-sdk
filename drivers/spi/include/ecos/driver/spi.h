#ifndef ECOS_DRIVER_SPI_H
#define ECOS_DRIVER_SPI_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_spi_id_t;
#define ECOS_SPI_DEFAULT ((ecos_spi_id_t)0u)

typedef enum {
    ECOS_SPI_MODE_0 = 0,
    ECOS_SPI_MODE_1,
    ECOS_SPI_MODE_2,
    ECOS_SPI_MODE_3,
} ecos_spi_mode_t;

typedef enum {
    ECOS_SPI_MSB_FIRST = 0,
    ECOS_SPI_LSB_FIRST,
} ecos_spi_bit_order_t;

typedef struct {
    uint32_t clock_hz;
    ecos_spi_mode_t mode;
    ecos_spi_bit_order_t bit_order;
    uint8_t cs_port;
    uint8_t cs_pin;
} ecos_spi_config_t;

#define ECOS_SPI_CONFIG_DEFAULT \
    { 1000000u, ECOS_SPI_MODE_0, ECOS_SPI_MSB_FIRST, 0u, 4u }

int ecos_spi_get_instance_count(void);
ecos_err_t ecos_spi_init(ecos_spi_id_t spi, const ecos_spi_config_t *config);
ecos_err_t ecos_spi_deinit(ecos_spi_id_t spi);
ecos_err_t ecos_spi_select(ecos_spi_id_t spi);
ecos_err_t ecos_spi_deselect(ecos_spi_id_t spi);
ecos_err_t ecos_spi_transfer(ecos_spi_id_t spi, const void *tx, void *rx, size_t size);
ecos_err_t ecos_spi_write(ecos_spi_id_t spi, const void *data, size_t size);
ecos_err_t ecos_spi_read(ecos_spi_id_t spi, void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
