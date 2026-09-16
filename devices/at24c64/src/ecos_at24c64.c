#include "ecos/device/at24c64.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define AT24C64_TRY(expression)                    \
    do {                                           \
        ecos_err_t at24c64_result = (expression);  \
        if (at24c64_result != ECOS_OK)             \
            return at24c64_result;                 \
    } while (0)

#define AT24C64_WORD_ADDRESS_BYTES 2u
#define AT24C64_WRITE_CYCLE_MS 5u

static ecos_err_t at24c64_check_range(uint16_t memory_address, size_t size)
{
    if ((uint32_t)memory_address + (uint32_t)size >
        ECOS_AT24C64_CAPACITY_BYTES)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_OK;
}

ecos_err_t ecos_at24c64_init(ecos_at24c64_t *eeprom,
                             const ecos_at24c64_config_t *config)
{
    int probe_result;

    if (eeprom == NULL || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    probe_result = ecos_i2c_probe(config->i2c, config->address);
    if (probe_result < 0)
        return (ecos_err_t)probe_result;
    if (probe_result == 0)
        return ECOS_ERR_NOT_FOUND;

    eeprom->config = *config;
    eeprom->initialized = 1u;
    return ECOS_OK;
}

ecos_err_t ecos_at24c64_deinit(ecos_at24c64_t *eeprom)
{
    if (eeprom == NULL || eeprom->initialized == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    eeprom->initialized = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_at24c64_read(ecos_at24c64_t *eeprom,
                             uint16_t memory_address,
                             void *data,
                             size_t size)
{
    uint8_t word_address[AT24C64_WORD_ADDRESS_BYTES];

    if (eeprom == NULL || data == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (eeprom->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if (size == 0u)
        return ECOS_OK;
    AT24C64_TRY(at24c64_check_range(memory_address, size));

    word_address[0] = (uint8_t)(memory_address >> 8);
    word_address[1] = (uint8_t)(memory_address & 0xFFu);
    return ecos_i2c_write_read(eeprom->config.i2c, eeprom->config.address,
                               word_address, sizeof(word_address),
                               data, size);
}

ecos_err_t ecos_at24c64_write(ecos_at24c64_t *eeprom,
                              uint16_t memory_address,
                              const void *data,
                              size_t size)
{
    uint8_t frame[AT24C64_WORD_ADDRESS_BYTES + ECOS_AT24C64_PAGE_BYTES];
    const uint8_t *cursor = (const uint8_t *)data;
    uint16_t address = memory_address;
    size_t remaining = size;

    if (eeprom == NULL || data == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (eeprom->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if (size == 0u)
        return ECOS_OK;
    AT24C64_TRY(at24c64_check_range(memory_address, size));

    while (remaining > 0u) {
        size_t page_room =
            ECOS_AT24C64_PAGE_BYTES - (address % ECOS_AT24C64_PAGE_BYTES);
        size_t chunk = remaining < page_room ? remaining : page_room;
        size_t index;

        frame[0] = (uint8_t)(address >> 8);
        frame[1] = (uint8_t)(address & 0xFFu);
        for (index = 0u; index < chunk; ++index)
            frame[AT24C64_WORD_ADDRESS_BYTES + index] = cursor[index];

        AT24C64_TRY(ecos_i2c_write(eeprom->config.i2c,
                                   eeprom->config.address,
                                   frame,
                                   AT24C64_WORD_ADDRESS_BYTES + chunk));
        AT24C64_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT,
                                        AT24C64_WRITE_CYCLE_MS));

        address = (uint16_t)(address + chunk);
        cursor += chunk;
        remaining -= chunk;
    }
    return ECOS_OK;
}
