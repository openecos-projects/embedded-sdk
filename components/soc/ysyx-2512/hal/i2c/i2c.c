#include "ecos/hal/i2c.h"
#include "ysyx_2512_soc.h"

#include <stddef.h>
#include <stdint.h>

#define YSYX_2512_I2C_COUNT 1u
#define I2C_GPIO0_PINS (((uint32_t)1u << 27) | ((uint32_t)1u << 28))
#define I2C_CONTROL_ENABLE 0x80u
#define I2C_COMMAND_START_WRITE 0x90u
#define I2C_COMMAND_STOP 0x40u
#define I2C_COMMAND_CLEAR_STATUS 0x01u
#define I2C_STATUS_RX_NACK 0x80u
#define I2C_STATUS_BUSY 0x40u
#define I2C_STATUS_ARBITRATION_LOST 0x20u
#define I2C_STATUS_TRANSFER_IN_PROGRESS 0x02u
#define I2C_STATUS_COMPLETE 0x01u
#define I2C_TRANSFER_TIMEOUT 100000u

static uint8_t i2c_initialized[YSYX_2512_I2C_COUNT];

static int i2c_id_is_valid(hal_i2c_id_t i2c)
{
    return i2c < YSYX_2512_I2C_COUNT;
}

static void i2c_configure_pins(void)
{
    REG_GPIO_0_IOFCFG |= I2C_GPIO0_PINS;
    REG_GPIO_0_PINMUX &= ~I2C_GPIO0_PINS;
}

static ecos_err_t i2c_wait_for_transfer(void)
{
    uint32_t timeout = I2C_TRANSFER_TIMEOUT;
    uint8_t transfer_started = 0u;

    while (timeout-- != 0u) {
        uint8_t status = REG_I2C_0_SR;

        if ((status & I2C_STATUS_ARBITRATION_LOST) != 0u)
            return ECOS_ERR_IO;
        if ((status & I2C_STATUS_TRANSFER_IN_PROGRESS) != 0u) {
            transfer_started = 1u;
        } else if (transfer_started != 0u ||
                   (status & I2C_STATUS_COMPLETE) != 0u) {
            return ECOS_OK;
        }
    }
    return ECOS_ERR_TIMEOUT;
}

static ecos_err_t i2c_stop(void)
{
    uint32_t timeout = I2C_TRANSFER_TIMEOUT;

    REG_I2C_0_CMD = I2C_COMMAND_CLEAR_STATUS;
    REG_I2C_0_CMD = I2C_COMMAND_STOP;
    while ((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

int hal_i2c_get_instance_count(void)
{
    return (int)YSYX_2512_I2C_COUNT;
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
    REG_I2C_0_PSCR = (uint8_t)(config->clock_divider - 1u);
    REG_I2C_0_CMD = I2C_COMMAND_CLEAR_STATUS;
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
    int acknowledged;

    if (!i2c_id_is_valid(i2c) || address > 0x7fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (i2c_initialized[i2c] == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    if ((REG_I2C_0_SR & I2C_STATUS_BUSY) != 0u)
        return ECOS_ERR_BUSY;

    REG_I2C_0_CMD = I2C_COMMAND_CLEAR_STATUS;
    REG_I2C_0_TXR = (uint8_t)(address << 1);
    REG_I2C_0_CMD = I2C_COMMAND_START_WRITE;

    result = i2c_wait_for_transfer();
    if (result != ECOS_OK) {
        (void)i2c_stop();
        return result;
    }

    acknowledged = (REG_I2C_0_SR & I2C_STATUS_RX_NACK) == 0u;
    result = i2c_stop();
    return result == ECOS_OK ? acknowledged : result;
}
