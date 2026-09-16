#include "ecos/hal/i2c.h"
#include "ysyx_2512_1_soc.h"

#include <stddef.h>
#include <stdint.h>

#define YSYX_2512_1_I2C_COUNT 1u
#define I2C_GPIO0_SCL_PIN 27u
#define I2C_GPIO0_SDA_PIN 28u
#define I2C_GPIO0_PINS (((uint32_t)1u << I2C_GPIO0_SCL_PIN) | \
                        ((uint32_t)1u << I2C_GPIO0_SDA_PIN))
#define I2C_CONTROL_ENABLE 0x80u
#define I2C_COMMAND_START_WRITE 0x90u
#define I2C_COMMAND_WRITE 0x10u
#define I2C_COMMAND_READ 0x20u
#define I2C_COMMAND_STOP 0x40u
#define I2C_COMMAND_STOP_READ 0x68u
#define I2C_STATUS_RX_NACK 0x80u
#define I2C_STATUS_BUSY 0x40u
#define I2C_STATUS_ARBITRATION_LOST 0x20u
#define I2C_STATUS_TRANSFER_IN_PROGRESS 0x02u
#define I2C_TRANSFER_TIMEOUT 100000u

static uint8_t i2c_initialized[YSYX_2512_1_I2C_COUNT];

static int i2c_id_is_valid(hal_i2c_id_t i2c)
{
    return i2c < YSYX_2512_1_I2C_COUNT;
}

static void i2c_configure_pins(void)
{
    REG_GPIO_0_IOFCFG |= I2C_GPIO0_PINS;
    REG_GPIO_0_PINMUX &= ~I2C_GPIO0_PINS;
}

static ecos_err_t i2c_wait_for_transfer(void)
{
    uint32_t timeout = I2C_TRANSFER_TIMEOUT;

    while ((REG_I2C_0_SR & I2C_STATUS_TRANSFER_IN_PROGRESS) == 0u) {
        if ((REG_I2C_0_SR & I2C_STATUS_ARBITRATION_LOST) != 0u)
            return ECOS_ERR_IO;
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    timeout = I2C_TRANSFER_TIMEOUT;
    while ((REG_I2C_0_SR & I2C_STATUS_TRANSFER_IN_PROGRESS) != 0u) {
        if ((REG_I2C_0_SR & I2C_STATUS_ARBITRATION_LOST) != 0u)
            return ECOS_ERR_IO;
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static ecos_err_t i2c_wait_idle(void)
{
    uint32_t timeout = I2C_TRANSFER_TIMEOUT;

    while ((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u) {
        if ((REG_I2C_0_SR & I2C_STATUS_ARBITRATION_LOST) != 0u)
            return ECOS_ERR_IO;
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static ecos_err_t i2c_stop(void)
{
    REG_I2C_0_CMD = I2C_COMMAND_STOP;
    return i2c_wait_idle();
}

static ecos_err_t i2c_send_byte(uint8_t value, uint8_t command)
{
    ecos_err_t result;

    REG_I2C_0_TXR = value;
    REG_I2C_0_CMD = command;
    result = i2c_wait_for_transfer();
    if (result != ECOS_OK)
        return result;
    if ((REG_I2C_0_SR & I2C_STATUS_RX_NACK) != 0u)
        return ECOS_ERR_NOT_FOUND;
    return ECOS_OK;
}

static ecos_err_t i2c_receive_byte(uint8_t command, uint8_t *value)
{
    ecos_err_t result;

    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_I2C_0_CMD = command;
    result = i2c_wait_for_transfer();
    if (result != ECOS_OK)
        return result;
    *value = REG_I2C_0_RXR;
    return ECOS_OK;
}

static ecos_err_t i2c_start(uint8_t address, uint8_t read_direction)
{
    return i2c_send_byte(
        (uint8_t)((address << 1) | (read_direction != 0u ? 1u : 0u)),
        I2C_COMMAND_START_WRITE
    );
}

int hal_i2c_get_instance_count(void)
{
    return (int)YSYX_2512_1_I2C_COUNT;
}

ecos_err_t hal_i2c_init(hal_i2c_id_t i2c,
                        const hal_i2c_config_t *config)
{
    if (!i2c_id_is_valid(i2c) || config == NULL ||
        config->clock_divider == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->clock_divider > 256u)
        return ECOS_ERR_UNSUPPORTED;

    i2c_configure_pins();
    REG_I2C_0_CTRL = 0u;
    REG_I2C_0_PSCR = config->clock_divider - 1u;
    REG_I2C_0_CTRL = I2C_CONTROL_ENABLE;
    i2c_initialized[i2c] = 1u;
    return ECOS_OK;
}

ecos_err_t hal_i2c_deinit(hal_i2c_id_t i2c)
{
    ecos_err_t result;

    if (!i2c_id_is_valid(i2c))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (i2c_initialized[i2c] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    result = i2c_stop();
    REG_I2C_0_CTRL = 0u;
    i2c_initialized[i2c] = 0u;
    return result;
}

int hal_i2c_probe(hal_i2c_id_t i2c, uint8_t address)
{
    ecos_err_t result;

    if (!i2c_id_is_valid(i2c) || address > 0x7fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (i2c_initialized[i2c] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if ((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u)
        return ECOS_ERR_BUSY;

    result = i2c_start(address, 0u);
    if (result == ECOS_ERR_NOT_FOUND) {
        result = i2c_stop();
        return result == ECOS_OK ? 0 : result;
    }
    if (result != ECOS_OK) {
        (void)i2c_stop();
        return result;
    }
    result = i2c_stop();
    return result == ECOS_OK ? 1 : result;
}

static ecos_err_t i2c_check_transfer_args(hal_i2c_id_t i2c,
                                          uint8_t address)
{
    if (!i2c_id_is_valid(i2c) || address > 0x7fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (i2c_initialized[i2c] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if ((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u)
        return ECOS_ERR_BUSY;
    return ECOS_OK;
}

ecos_err_t hal_i2c_write(hal_i2c_id_t i2c,
                         uint8_t address,
                         const uint8_t *data,
                         size_t size)
{
    ecos_err_t result;
    size_t index;

    if (data == NULL || size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = i2c_check_transfer_args(i2c, address);
    if (result != ECOS_OK)
        return result;

    result = i2c_start(address, 0u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < size; ++index) {
        result = i2c_send_byte(data[index], I2C_COMMAND_WRITE);
        if (result != ECOS_OK)
            goto fail;
    }
    return i2c_stop();

fail:
    (void)i2c_stop();
    return result;
}

ecos_err_t hal_i2c_read(hal_i2c_id_t i2c,
                        uint8_t address,
                        uint8_t *data,
                        size_t size)
{
    ecos_err_t result;
    size_t index;

    if (data == NULL || size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = i2c_check_transfer_args(i2c, address);
    if (result != ECOS_OK)
        return result;

    result = i2c_start(address, 1u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < size; ++index) {
        const uint8_t command = index + 1u == size ?
                                I2C_COMMAND_STOP_READ : I2C_COMMAND_READ;
        result = i2c_receive_byte(command, &data[index]);
        if (result != ECOS_OK)
            goto fail;
    }
    return i2c_wait_idle();

fail:
    (void)i2c_stop();
    return result;
}

ecos_err_t hal_i2c_write_read(hal_i2c_id_t i2c,
                              uint8_t address,
                              const uint8_t *write_data,
                              size_t write_size,
                              uint8_t *read_data,
                              size_t read_size)
{
    ecos_err_t result;
    size_t index;

    if (write_data == NULL || write_size == 0u ||
        read_data == NULL || read_size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = i2c_check_transfer_args(i2c, address);
    if (result != ECOS_OK)
        return result;

    result = i2c_start(address, 0u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < write_size; ++index) {
        result = i2c_send_byte(write_data[index], I2C_COMMAND_WRITE);
        if (result != ECOS_OK)
            goto fail;
    }

    result = i2c_start(address, 1u);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < read_size; ++index) {
        const uint8_t command = index + 1u == read_size ?
                                I2C_COMMAND_STOP_READ : I2C_COMMAND_READ;
        result = i2c_receive_byte(command, &read_data[index]);
        if (result != ECOS_OK)
            goto fail;
    }
    return i2c_wait_idle();

fail:
    (void)i2c_stop();
    return result;
}
