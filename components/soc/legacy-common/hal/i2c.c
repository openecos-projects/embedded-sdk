#include "ecos/hal/i2c.h"
#include "legacy_soc.h"

#include <stddef.h>

#define I2C_CONTROL_ENABLE 0x80u
#define I2C_COMMAND_START_WRITE 0x90u
#define I2C_COMMAND_WRITE 0x10u
#define I2C_COMMAND_READ 0x20u
#define I2C_COMMAND_STOP 0x40u
#define I2C_COMMAND_STOP_READ 0x60u
#define I2C_STATUS_RX_NACK 0x80u
#define I2C_STATUS_BUSY 0x40u
#define I2C_STATUS_ARBITRATION_LOST 0x20u
#define I2C_STATUS_TRANSFER 0x02u
#define I2C_TIMEOUT 100000u

static uint8_t initialized;

static ecos_err_t wait_transfer(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while ((REG_I2C_0_SR & I2C_STATUS_TRANSFER) == 0u) {
        if ((REG_I2C_0_SR & I2C_STATUS_ARBITRATION_LOST) != 0u)
            return ECOS_ERR_IO;
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    timeout = I2C_TIMEOUT;
    while ((REG_I2C_0_SR & I2C_STATUS_TRANSFER) != 0u) {
        if ((REG_I2C_0_SR & I2C_STATUS_ARBITRATION_LOST) != 0u)
            return ECOS_ERR_IO;
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static ecos_err_t wait_idle(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while ((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static void configure_pins(void)
{
#if LEGACY_HAS_GPIO_MUX
    const uint32_t pins = ((uint32_t)1u << 29) | ((uint32_t)1u << 30);
    REG_GPIO_0_IOFCFG |= pins;
    REG_GPIO_0_PINMUX &= ~pins;
#endif
}

static ecos_err_t check_transfer(uint8_t address)
{
    if (address > 0x7fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if ((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u)
        return ECOS_ERR_BUSY;
    return ECOS_OK;
}

static ecos_err_t send_byte(uint8_t value, uint8_t command)
{
    ecos_err_t result;
    REG_I2C_0_TXR = value;
    REG_I2C_0_CMD = command;
    result = wait_transfer();
    if (result != ECOS_OK)
        return result;
    return (REG_I2C_0_SR & I2C_STATUS_RX_NACK) != 0u ?
           ECOS_ERR_NOT_FOUND : ECOS_OK;
}

static ecos_err_t receive_byte(uint8_t command, uint8_t *value)
{
    ecos_err_t result;
    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_I2C_0_CMD = command;
    result = wait_transfer();
    if (result != ECOS_OK)
        return result;
    *value = REG_I2C_0_RXR;
    return ECOS_OK;
}

static ecos_err_t start_transaction(uint8_t address, uint8_t read)
{
    return send_byte((uint8_t)((address << 1) | (read != 0u ? 1u : 0u)),
                     I2C_COMMAND_START_WRITE);
}

static ecos_err_t stop_transaction(void)
{
    REG_I2C_0_CMD = I2C_COMMAND_STOP;
    return wait_idle();
}

int hal_i2c_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_i2c_init(hal_i2c_id_t i2c, const hal_i2c_config_t *config)
{
    if (i2c != 0u || config == NULL || config->clock_divider == 0u ||
        config->clock_divider > 256u)
        return ECOS_ERR_INVALID_ARGUMENT;
    configure_pins();
    REG_I2C_0_CTRL = 0u;
    REG_I2C_0_PSCR = config->clock_divider - 1u;
    REG_I2C_0_CTRL = I2C_CONTROL_ENABLE;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_i2c_deinit(hal_i2c_id_t i2c)
{
    ecos_err_t result;
    if (i2c != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = stop_transaction();
    REG_I2C_0_CTRL = 0u;
    initialized = 0u;
    return result;
}

int hal_i2c_probe(hal_i2c_id_t i2c, uint8_t address)
{
    ecos_err_t result;
    if (i2c != 0u || address > 0x7fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = check_transfer(address);
    if (result != ECOS_OK)
        return result;
    result = start_transaction(address, 0u);
    if (result == ECOS_ERR_NOT_FOUND) {
        (void)stop_transaction();
        return 0;
    }
    if (result != ECOS_OK) {
        (void)stop_transaction();
        return result;
    }
    result = stop_transaction();
    return result == ECOS_OK ? 1 : result;
}

ecos_err_t hal_i2c_write(hal_i2c_id_t i2c, uint8_t address,
                         const uint8_t *data, size_t size)
{
    ecos_err_t result;
    size_t index;
    if (data == NULL || size == 0u || i2c != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = check_transfer(address);
    if (result != ECOS_OK)
        return result;
    result = start_transaction(address, 0u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < size; ++index) {
        result = send_byte(data[index], I2C_COMMAND_WRITE);
        if (result != ECOS_OK)
            goto fail;
    }
    return stop_transaction();
fail:
    (void)stop_transaction();
    return result;
}

ecos_err_t hal_i2c_read(hal_i2c_id_t i2c, uint8_t address,
                        uint8_t *data, size_t size)
{
    ecos_err_t result;
    size_t index;
    if (data == NULL || size == 0u || i2c != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = check_transfer(address);
    if (result != ECOS_OK)
        return result;
    result = start_transaction(address, 1u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < size; ++index) {
        uint8_t command = index + 1u == size ?
                          I2C_COMMAND_STOP_READ : I2C_COMMAND_READ;
        result = receive_byte(command, &data[index]);
        if (result != ECOS_OK)
            goto fail;
    }
    return wait_idle();
fail:
    (void)stop_transaction();
    return result;
}

ecos_err_t hal_i2c_write_read(hal_i2c_id_t i2c, uint8_t address,
                              const uint8_t *write_data, size_t write_size,
                              uint8_t *read_data, size_t read_size)
{
    ecos_err_t result;
    size_t index;
    if (i2c != 0u || write_data == NULL || write_size == 0u ||
        read_data == NULL || read_size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = check_transfer(address);
    if (result != ECOS_OK)
        return result;
    result = start_transaction(address, 0u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < write_size; ++index) {
        result = send_byte(write_data[index], I2C_COMMAND_WRITE);
        if (result != ECOS_OK)
            goto fail;
    }
    result = start_transaction(address, 1u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < read_size; ++index) {
        uint8_t command = index + 1u == read_size ?
                          I2C_COMMAND_STOP_READ : I2C_COMMAND_READ;
        result = receive_byte(command, &read_data[index]);
        if (result != ECOS_OK)
            goto fail;
    }
    return wait_idle();
fail:
    (void)stop_transaction();
    return result;
}
