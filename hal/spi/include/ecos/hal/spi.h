#ifndef ECOS_HAL_SPI_H
#define ECOS_HAL_SPI_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_spi_id_t;

typedef enum {
    HAL_SPI_MODE_0 = 0,
    HAL_SPI_MODE_1,
    HAL_SPI_MODE_2,
    HAL_SPI_MODE_3,
} hal_spi_mode_t;

typedef enum {
    HAL_SPI_BIT_ORDER_MSB_FIRST = 0,
    HAL_SPI_BIT_ORDER_LSB_FIRST,
} hal_spi_bit_order_t;

typedef struct {
    uint32_t clock_hz;
    hal_spi_mode_t mode;
    hal_spi_bit_order_t bit_order;
} hal_spi_config_t;

int hal_spi_get_instance_count(void);
ecos_err_t hal_spi_init(hal_spi_id_t spi, const hal_spi_config_t *config);
ecos_err_t hal_spi_deinit(hal_spi_id_t spi);

/* Transfer one byte while the caller-selected CS remains asserted. */
ecos_err_t hal_spi_transfer_byte(hal_spi_id_t spi, uint8_t tx, uint8_t *rx);

#ifdef __cplusplus
}
#endif

#endif
