/* CH32V103 I2C1 polling master.
 * Default pins: PB6=SCL and PB7=SDA, open-drain alternate function.
 * The public clock_divider is the desired APB1/input clock divider; the
 * standard peripheral block is programmed for standard-mode I2C.
 */
#include "ecos/hal/i2c.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define RCC_APB2PCENR REG32(0x40021018u)
#define RCC_APB1PCENR REG32(0x4002101Cu)
#define AFIO_PCFR1    REG32(0x40010004u)
#define GPIOB_CFGLR   REG32(0x40010C00u)
#define I2C1_BASE     0x40005400u
#define I2C_CTLR1     REG16(I2C1_BASE + 0x00u)
#define I2C_CTLR2     REG16(I2C1_BASE + 0x04u)
#define I2C_OADDR1    REG16(I2C1_BASE + 0x08u)
#define I2C_DATAR     REG16(I2C1_BASE + 0x10u)
#define I2C_STAR1     REG16(I2C1_BASE + 0x14u)
#define I2C_STAR2     REG16(I2C1_BASE + 0x18u)
#define I2C_CKCFGR    REG16(I2C1_BASE + 0x1Cu)

#define I2C_CR1_PE    0x0001u
#define I2C_CR1_START 0x0100u
#define I2C_CR1_STOP  0x0200u
#define I2C_CR1_ACK   0x0400u
#define I2C_SR1_SB    0x0001u
#define I2C_SR1_ADDR  0x0002u
#define I2C_SR1_BTF   0x0004u
#define I2C_SR1_RXNE  0x0040u
#define I2C_SR1_TXE   0x0080u
#define I2C_SR1_BERR  0x0100u
#define I2C_SR1_ARLO  0x0200u
#define I2C_SR1_AF    0x0400u
#define I2C_SR2_MSL   0x0001u
#define I2C_SR2_BUSY  0x0002u
#define I2C_SR2_TRA   0x0004u
#define I2C_TIMEOUT   200000u
#define I2C_APB1_HZ   8000000u
#define I2C_MIN_CCR   4u

static uint8_t initialized;

static int valid_id(hal_i2c_id_t id)
{
    return id == 0u;
}

static void enable_clocks(void)
{
    RCC_APB2PCENR |= (1u << 0) | (1u << 3); /* AFIO, GPIOB */
    RCC_APB1PCENR |= (1u << 21);             /* I2C1 */
}

static void configure_pins(void)
{
    /* PB6/PB7: alternate-function open-drain, 50 MHz (CNF=11, MODE=11). */
    GPIOB_CFGLR = (GPIOB_CFGLR & ~0xFF000000u) | 0xFF000000u;
    AFIO_PCFR1 &= ~(1u << 1); /* I2C1 no remap: PB6/PB7 */
}

static int wait_status(uint16_t mask, int set)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((((uint16_t)I2C_STAR1 & mask) != 0u) != (set != 0)) {
        uint16_t error = I2C_STAR1;
        if ((error & (I2C_SR1_BERR | I2C_SR1_ARLO)) != 0u)
            return ECOS_ERR_IO;
        if ((error & I2C_SR1_AF) != 0u)
            return ECOS_ERR_NOT_FOUND;
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static ecos_err_t wait_bus_idle(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while ((I2C_STAR2 & I2C_SR2_BUSY) != 0u) {
        if (I2C_STAR1 & (I2C_SR1_BERR | I2C_SR1_ARLO))
            return ECOS_ERR_IO;
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static void clear_addr(void)
{
    volatile uint16_t sr1 = I2C_STAR1;
    volatile uint16_t sr2 = I2C_STAR2;
    (void)sr1;
    (void)sr2;
}

static ecos_err_t start_address(uint8_t address, int read)
{
    ecos_err_t result;
    I2C_CTLR1 |= I2C_CR1_START;
    result = wait_status(I2C_SR1_SB, 1);
    if (result != ECOS_OK)
        return result;
    I2C_DATAR = (uint16_t)((address << 1) | (read ? 1u : 0u));
    result = wait_status(I2C_SR1_ADDR, 1);
    if (result != ECOS_OK)
        return result;
    clear_addr();
    return ECOS_OK;
}

static ecos_err_t stop_bus(void)
{
    I2C_CTLR1 |= I2C_CR1_STOP;
    return wait_bus_idle();
}

static ecos_err_t check(hal_i2c_id_t id, uint8_t address)
{
    if (!valid_id(id) || address > 0x7Fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    return wait_bus_idle();
}

int hal_i2c_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_i2c_init(hal_i2c_id_t id, const hal_i2c_config_t *config)
{
    uint32_t divider;
    uint16_t ccr;

    if (!valid_id(id) || config == NULL || config->clock_divider < 2u ||
        config->clock_divider > 8190u)
        return ECOS_ERR_INVALID_ARGUMENT;
    enable_clocks();
    configure_pins();
    /* clock_divider is the public divider parameter; the CH32 CCR is half
     * of it for standard-mode SCL (8 MHz / (2 * CCR)). */
    divider = config->clock_divider;
    ccr = (uint16_t)(divider / 2u);
    if (ccr < I2C_MIN_CCR)
        ccr = I2C_MIN_CCR;
    if (ccr > 0x0FFFu)
        return ECOS_ERR_UNSUPPORTED;
    I2C_CTLR1 = 0u;
    I2C_CTLR2 = 8u; /* APB1 frequency in MHz */
    I2C_CKCFGR = ccr;
    I2C_OADDR1 = 0x4000u; /* 7-bit master mode, own address disabled */
    I2C_CTLR1 = I2C_CR1_PE | I2C_CR1_ACK;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_i2c_deinit(hal_i2c_id_t id)
{
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    (void)stop_bus();
    I2C_CTLR1 = 0u;
    initialized = 0u;
    return ECOS_OK;
}

int hal_i2c_probe(hal_i2c_id_t id, uint8_t address)
{
    ecos_err_t result = check(id, address);
    if (result != ECOS_OK)
        return result;
    result = start_address(address, 0);
    if (result == ECOS_ERR_NOT_FOUND) {
        (void)stop_bus();
        return 0;
    }
    if (result != ECOS_OK) {
        (void)stop_bus();
        return result;
    }
    result = stop_bus();
    return result == ECOS_OK ? 1 : result;
}

ecos_err_t hal_i2c_write(hal_i2c_id_t id, uint8_t address,
                         const uint8_t *data, size_t size)
{
    ecos_err_t result;
    size_t index;
    if (data == NULL || size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = check(id, address);
    if (result != ECOS_OK)
        return result;
    result = start_address(address, 0);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < size; ++index) {
        result = wait_status(I2C_SR1_TXE, 1);
        if (result != ECOS_OK)
            goto fail;
        I2C_DATAR = data[index];
    }
    result = wait_status(I2C_SR1_BTF, 1);
    if (result != ECOS_OK)
        goto fail;
    return stop_bus();
fail:
    (void)stop_bus();
    return result;
}

ecos_err_t hal_i2c_read(hal_i2c_id_t id, uint8_t address,
                        uint8_t *data, size_t size)
{
    ecos_err_t result;
    size_t index;
    if (data == NULL || size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = check(id, address);
    if (result != ECOS_OK)
        return result;
    result = start_address(address, 1);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < size; ++index) {
        if (index + 1u == size)
            I2C_CTLR1 &= (uint16_t)~I2C_CR1_ACK;
        result = wait_status(I2C_SR1_RXNE, 1);
        if (result != ECOS_OK)
            goto fail;
        data[index] = (uint8_t)I2C_DATAR;
    }
    I2C_CTLR1 |= I2C_CR1_ACK;
    return stop_bus();
fail:
    (void)stop_bus();
    return result;
}

ecos_err_t hal_i2c_write_read(hal_i2c_id_t id, uint8_t address,
                              const uint8_t *write_data, size_t write_size,
                              uint8_t *read_data, size_t read_size)
{
    ecos_err_t result;
    size_t index;
    if (write_data == NULL || write_size == 0u ||
        read_data == NULL || read_size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = check(id, address);
    if (result != ECOS_OK)
        return result;
    result = start_address(address, 0);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < write_size; ++index) {
        result = wait_status(I2C_SR1_TXE, 1);
        if (result != ECOS_OK)
            goto fail;
        I2C_DATAR = write_data[index];
    }
    result = wait_status(I2C_SR1_BTF, 1);
    if (result != ECOS_OK)
        goto fail;
    result = start_address(address, 1);
    if (result != ECOS_OK)
        goto fail;
    for (index = 0u; index < read_size; ++index) {
        if (index + 1u == read_size)
            I2C_CTLR1 &= (uint16_t)~I2C_CR1_ACK;
        result = wait_status(I2C_SR1_RXNE, 1);
        if (result != ECOS_OK)
            goto fail;
        read_data[index] = (uint8_t)I2C_DATAR;
    }
    I2C_CTLR1 |= I2C_CR1_ACK;
    return stop_bus();
fail:
    (void)stop_bus();
    return result;
}
