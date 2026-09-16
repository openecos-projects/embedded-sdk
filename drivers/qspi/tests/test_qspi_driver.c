#include "ecos/driver/qspi.h"
#include "ecos/hal/qspi.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static int init_calls;
static int write8_calls;
static int send_calls;
static int write_calls;
static int read_calls;
static hal_qspi_cs_t last_cs;
static uint16_t last_length;

int hal_qspi_init(hal_qspi_port_t port, const hal_qspi_config_t *config)
{
    assert(port == HAL_QSPI_PORT_0);
    assert(config != NULL && config->clkdiv == 3u);
    ++init_calls;
    return 0;
}

int hal_qspi_deinit(hal_qspi_port_t port)
{
    assert(port == HAL_QSPI_PORT_0);
    return 0;
}

int hal_qspi_write_8_cs(hal_qspi_port_t port, uint8_t data, hal_qspi_cs_t cs)
{
    assert(port == HAL_QSPI_PORT_0 && data == 0x5Au);
    ++write8_calls;
    last_cs = cs;
    return 0;
}

int hal_qspi_send_cmd(hal_qspi_port_t port, uint8_t cmd, uint8_t cmd_len,
                      uint32_t addr, uint8_t addr_len)
{
    assert(port == HAL_QSPI_PORT_0 && cmd == 0x9Fu && cmd_len == 8u);
    assert(addr == 0x123456u && addr_len == 24u);
    ++send_calls;
    return 0;
}

int hal_qspi_write(hal_qspi_port_t port, uint8_t cmd, uint8_t cmd_len,
                   uint32_t addr, uint8_t addr_len, const uint8_t *data,
                   uint16_t length)
{
    assert(port == HAL_QSPI_PORT_0 && cmd == 0x02u && cmd_len == 8u);
    assert(addr == 0x100u && addr_len == 24u && data != NULL);
    ++write_calls;
    last_length = length;
    return 0;
}

int hal_qspi_read(hal_qspi_port_t port, uint8_t cmd, uint8_t cmd_len,
                  uint32_t addr, uint8_t addr_len, uint8_t dummy_cycles,
                  uint8_t *data, uint16_t length)
{
    assert(port == HAL_QSPI_PORT_0 && cmd == 0x0Bu && cmd_len == 8u);
    assert(addr == 0x200u && addr_len == 24u && dummy_cycles == 8u);
    assert(data != NULL);
    ++read_calls;
    last_length = length;
    data[0] = 0xA5u;
    return 0;
}

int main(void)
{
    const ecos_qspi_config_t config = { 3u };
    uint8_t tx = 0x42u;
    uint8_t rx = 0u;

    assert(ecos_qspi_get_instance_count() == 1);
    assert(ecos_qspi_init(ECOS_QSPI_DEFAULT, &config) == ECOS_OK);
    assert(init_calls == 1);
    assert(ecos_qspi_init(1u, &config) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_qspi_init(ECOS_QSPI_DEFAULT, NULL) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_qspi_write_8_cs(ECOS_QSPI_DEFAULT, 0x5Au,
                                 ECOS_QSPI_CS_2) == ECOS_OK);
    assert(write8_calls == 1 && last_cs == HAL_QSPI_CS_2);
    assert(ecos_qspi_write_8_cs(ECOS_QSPI_DEFAULT, 0x5Au,
                                 ECOS_QSPI_CS_COUNT) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_qspi_send_cmd(ECOS_QSPI_DEFAULT, 0x9Fu, 8u,
                              0x123456u, 24u) == ECOS_OK);
    assert(send_calls == 1);
    assert(ecos_qspi_send_cmd(ECOS_QSPI_DEFAULT, 0x9Fu, 9u,
                              0x123456u, 24u) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_qspi_write(ECOS_QSPI_DEFAULT, 0x02u, 8u, 0x100u, 24u,
                           &tx, 1u) == ECOS_OK);
    assert(write_calls == 1 && last_length == 1u);
    assert(ecos_qspi_write(ECOS_QSPI_DEFAULT, 0x02u, 8u, 0x100u, 24u,
                           NULL, 1u) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_qspi_read(ECOS_QSPI_DEFAULT, 0x0Bu, 8u, 0x200u, 24u,
                          8u, &rx, 1u) == ECOS_OK);
    assert(read_calls == 1 && last_length == 1u && rx == 0xA5u);
    assert(ecos_qspi_read(ECOS_QSPI_DEFAULT, 0x0Bu, 8u, 0x200u, 24u,
                          8u, NULL, 1u) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_qspi_deinit(ECOS_QSPI_DEFAULT) == ECOS_OK);
    return 0;
}
