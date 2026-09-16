#include "ecos/driver/i2c.h"
#include "ecos/hal/i2c.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static unsigned init_calls;
static unsigned deinit_calls;
static unsigned probe_calls;
static unsigned write_calls;
static unsigned read_calls;
static unsigned write_read_calls;
static uint8_t last_address;
static const uint8_t *last_write_data;
static size_t last_write_size;
static uint8_t *last_read_data;
static size_t last_read_size;

int hal_i2c_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_i2c_init(hal_i2c_id_t i2c,
                        const hal_i2c_config_t *config)
{
    assert(i2c == 0u);
    assert(config != NULL && config->clock_divider == 100u);
    ++init_calls;
    return ECOS_OK;
}

ecos_err_t hal_i2c_deinit(hal_i2c_id_t i2c)
{
    assert(i2c == 0u);
    ++deinit_calls;
    return ECOS_OK;
}

int hal_i2c_probe(hal_i2c_id_t i2c, uint8_t address)
{
    assert(i2c == 0u);
    last_address = address;
    ++probe_calls;
    return 1;
}

ecos_err_t hal_i2c_write(hal_i2c_id_t i2c,
                         uint8_t address,
                         const uint8_t *data,
                         size_t size)
{
    assert(i2c == 0u);
    last_address = address;
    last_write_data = data;
    last_write_size = size;
    ++write_calls;
    return ECOS_OK;
}

ecos_err_t hal_i2c_read(hal_i2c_id_t i2c,
                        uint8_t address,
                        uint8_t *data,
                        size_t size)
{
    assert(i2c == 0u);
    last_address = address;
    last_read_data = data;
    last_read_size = size;
    ++read_calls;
    return ECOS_OK;
}

ecos_err_t hal_i2c_write_read(hal_i2c_id_t i2c,
                              uint8_t address,
                              const uint8_t *write_data,
                              size_t write_size,
                              uint8_t *read_data,
                              size_t read_size)
{
    assert(i2c == 0u);
    last_address = address;
    last_write_data = write_data;
    last_write_size = write_size;
    last_read_data = read_data;
    last_read_size = read_size;
    ++write_read_calls;
    return ECOS_OK;
}

int main(void)
{
    const uint8_t write_data[] = { 0x10u, 0x20u };
    uint8_t read_data[2] = { 0u, 0u };
    ecos_i2c_config_t config = { 100u };

    assert(ecos_i2c_get_instance_count() == 1);
    assert(ecos_i2c_init(0u, &config) == ECOS_OK);
    assert(init_calls == 1u);
    assert(ecos_i2c_deinit(0u) == ECOS_OK);
    assert(deinit_calls == 1u);

    assert(ecos_i2c_probe(0u, 0x42u) == 1);
    assert(probe_calls == 1u && last_address == 0x42u);
    assert(ecos_i2c_probe(0u, 0x80u) == ECOS_ERR_INVALID_ARGUMENT);
    assert(probe_calls == 1u);

    assert(ecos_i2c_write(0u, 0x42u, write_data, sizeof(write_data)) == ECOS_OK);
    assert(write_calls == 1u && last_address == 0x42u);
    assert(last_write_data == write_data && last_write_size == sizeof(write_data));
    assert(ecos_i2c_write(0u, 0x80u, write_data, sizeof(write_data)) ==
           ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_i2c_write(0u, 0x42u, NULL, 1u) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_i2c_write(0u, 0x42u, write_data, 0u) == ECOS_ERR_INVALID_ARGUMENT);

    assert(ecos_i2c_read(0u, 0x42u, read_data, sizeof(read_data)) == ECOS_OK);
    assert(read_calls == 1u && last_address == 0x42u);
    assert(last_read_data == read_data && last_read_size == sizeof(read_data));
    assert(ecos_i2c_read(0u, 0x80u, read_data, sizeof(read_data)) ==
           ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_i2c_read(0u, 0x42u, NULL, sizeof(read_data)) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_i2c_read(0u, 0x42u, read_data, 0u) == ECOS_ERR_INVALID_ARGUMENT);

    assert(ecos_i2c_write_read(
        0u, 0x42u, write_data, sizeof(write_data), read_data, sizeof(read_data)
    ) == ECOS_OK);
    assert(write_read_calls == 1u && last_address == 0x42u);
    assert(last_write_data == write_data && last_write_size == sizeof(write_data));
    assert(last_read_data == read_data && last_read_size == sizeof(read_data));
    assert(ecos_i2c_write_read(
        0u, 0x80u, write_data, sizeof(write_data), read_data, sizeof(read_data)
    ) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_i2c_write_read(
        0u, 0x42u, write_data, 0u, read_data, sizeof(read_data)
    ) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_i2c_write_read(
        0u, 0x42u, write_data, sizeof(write_data), NULL, sizeof(read_data)
    ) == ECOS_ERR_INVALID_ARGUMENT);
    assert(write_read_calls == 1u);

    return 0;
}
