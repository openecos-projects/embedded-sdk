#ifndef ECOS_DEVICE_AT24C64_H
#define ECOS_DEVICE_AT24C64_H

#include "ecos/driver/i2c.h"
#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AT24C64 7-bit I2C address with A2/A1/A0 tied low. */
#define ECOS_AT24C64_I2C_ADDRESS 0x50u
#define ECOS_AT24C64_CAPACITY_BYTES 8192u
#define ECOS_AT24C64_PAGE_BYTES 32u

typedef struct {
    ecos_i2c_id_t i2c;
    uint8_t address;
} ecos_at24c64_config_t;

typedef struct {
    ecos_at24c64_config_t config;
    uint8_t initialized;
} ecos_at24c64_t;

#define ECOS_AT24C64_CONFIG_DEFAULT \
    { ECOS_I2C_DEFAULT, ECOS_AT24C64_I2C_ADDRESS }

/* The I2C controller must be initialized by the caller before
 * ecos_at24c64_init; init probes the device address. */
ecos_err_t ecos_at24c64_init(ecos_at24c64_t *eeprom,
                             const ecos_at24c64_config_t *config);
ecos_err_t ecos_at24c64_deinit(ecos_at24c64_t *eeprom);

ecos_err_t ecos_at24c64_read(ecos_at24c64_t *eeprom,
                             uint16_t memory_address,
                             void *data,
                             size_t size);

/* Splits the transfer at 32-byte page boundaries and waits out the
 * internal write cycle after each page. */
ecos_err_t ecos_at24c64_write(ecos_at24c64_t *eeprom,
                              uint16_t memory_address,
                              const void *data,
                              size_t size);

#ifdef __cplusplus
}
#endif

#endif
