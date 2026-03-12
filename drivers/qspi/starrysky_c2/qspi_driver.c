/**
 * @file qspi_driver.c
 * @brief StarrySky C2 QSPI driver implementation
 */

#include "qspi.h"
#include "board.h"  // Include C2 board header for register definitions

// Static function declarations
static status_t qspi_init_impl(const qspi_config_t* config);
static status_t qspi_write_8_impl(uint8_t data);
static status_t qspi_write_16_impl(uint16_t data);
static status_t qspi_write_32_impl(uint32_t data);
static status_t qspi_read_8_impl(uint8_t* data);
static status_t qspi_read_16_impl(uint16_t* data);
static status_t qspi_read_32_impl(uint32_t* data);
static status_t qspi_transfer_impl(const uint8_t* tx_data, uint8_t* rx_data, size_t length);
static status_t qspi_write_8_cs_impl(uint8_t data, qspi_cs_t cs);
static status_t qspi_write_32_cs_impl(uint32_t data, qspi_cs_t cs);
static status_t qspi_dma_write_impl(const uint32_t* data, size_t count);
static status_t qspi_dma_read_impl(uint32_t* data, size_t count);
static status_t qspi_is_busy_impl(bool_t* busy);

// QSPI driver instance for StarrySky C2
static qspi_driver_t starrysky_c2_qspi_driver = {
    .base = {
        .version = DRIVER_VERSION(1, 0),
        .name = "starrysky_c2_qspi"
    },
    .init = qspi_init_impl,
    .write_8 = qspi_write_8_impl,
    .write_16 = qspi_write_16_impl,
    .write_32 = qspi_write_32_impl,
    .read_8 = qspi_read_8_impl,
    .read_16 = qspi_read_16_impl,
    .read_32 = qspi_read_32_impl,
    .transfer = qspi_transfer_impl,
    .write_8_cs = qspi_write_8_cs_impl,
    .write_32_cs = qspi_write_32_cs_impl,
    .dma_write = qspi_dma_write_impl,
    .dma_read = qspi_dma_read_impl,
    .is_busy = qspi_is_busy_impl
};

status_t starrysky_c2_qspi_init(void)
{
    return qspi_register_driver(&starrysky_c2_qspi_driver);
}

static status_t qspi_init_impl(const qspi_config_t* config)
{
    CHECK_NULL(config);
    
    // Set clock divisor
    REG_QSPI_0_CLKDIV = config->clkdiv;
    
    return STATUS_SUCCESS;
}

// Helper function to wait for QSPI not busy
static void wait_for_qspi_ready(void)
{
    while (REG_QSPI_0_STATUS & (1 << 0)); // Wait for busy bit to clear
}

static status_t qspi_write_8_impl(uint8_t data)
{
    wait_for_qspi_ready();
    
    // Set command for 8-bit write
    REG_QSPI_0_CMD = (1 << 0); // Write command
    
    // Write data
    REG_QSPI_0_TXFIFO = (uint32_t)data;
    
    return STATUS_SUCCESS;
}

static status_t qspi_write_16_impl(uint16_t data)
{
    wait_for_qspi_ready();
    
    // Set command for 16-bit write
    REG_QSPI_0_CMD = (1 << 1); // 16-bit write command
    
    // Write data
    REG_QSPI_0_TXFIFO = (uint32_t)data;
    
    return STATUS_SUCCESS;
}

static status_t qspi_write_32_impl(uint32_t data)
{
    wait_for_qspi_ready();
    
    // Set command for 32-bit write
    REG_QSPI_0_CMD = (1 << 2); // 32-bit write command
    
    // Write data
    REG_QSPI_0_TXFIFO = data;
    
    return STATUS_SUCCESS;
}

static status_t qspi_read_8_impl(uint8_t* data)
{
    CHECK_NULL(data);
    
    wait_for_qspi_ready();
    
    // Set command for 8-bit read
    REG_QSPI_0_CMD = (1 << 3); // Read command
    
    // Trigger read
    REG_QSPI_0_CMD |= (1 << 7); // Start transfer
    
    wait_for_qspi_ready();
    
    // Read data
    *data = (uint8_t)(REG_QSPI_0_RXFIFO & 0xFF);
    
    return STATUS_SUCCESS;
}

static status_t qspi_read_16_impl(uint16_t* data)
{
    CHECK_NULL(data);
    
    wait_for_qspi_ready();
    
    // Set command for 16-bit read
    REG_QSPI_0_CMD = (1 << 4); // 16-bit read command
    
    // Trigger read
    REG_QSPI_0_CMD |= (1 << 7); // Start transfer
    
    wait_for_qspi_ready();
    
    // Read data
    *data = (uint16_t)(REG_QSPI_0_RXFIFO & 0xFFFF);
    
    return STATUS_SUCCESS;
}

static status_t qspi_read_32_impl(uint32_t* data)
{
    CHECK_NULL(data);
    
    wait_for_qspi_ready();
    
    // Set command for 32-bit read
    REG_QSPI_0_CMD = (1 << 5); // 32-bit read command
    
    // Trigger read
    REG_QSPI_0_CMD |= (1 << 7); // Start transfer
    
    wait_for_qspi_ready();
    
    // Read data
    *data = REG_QSPI_0_RXFIFO;
    
    return STATUS_SUCCESS;
}

static status_t qspi_transfer_impl(const uint8_t* tx_data, uint8_t* rx_data, size_t length)
{
    CHECK_NULL(tx_data);
    CHECK_NULL(rx_data);
    
    if (length == 0) {
        return STATUS_INVALID_ARG;
    }
    
    // For simplicity, handle single byte transfers
    // In a real implementation, this would handle multi-byte transfers
    if (length == 1) {
        qspi_write_8_impl(*tx_data);
        return qspi_read_8_impl(rx_data);
    }
    
    // Handle multiple bytes
    for (size_t i = 0; i < length; i++) {
        qspi_write_8_impl(tx_data[i]);
        qspi_read_8_impl(&rx_data[i]);
    }
    
    return STATUS_SUCCESS;
}

static status_t qspi_write_8_cs_impl(uint8_t data, qspi_cs_t cs)
{
    wait_for_qspi_ready();
    
    // Set chip select
    REG_QSPI_0_CMD = (1 << 0) | (cs & 0x030000); // Write command with CS
    
    // Write data
    REG_QSPI_0_TXFIFO = (uint32_t)data;
    
    return STATUS_SUCCESS;
}

static status_t qspi_write_32_cs_impl(uint32_t data, qspi_cs_t cs)
{
    wait_for_qspi_ready();
    
    // Set chip select and 32-bit write command
    REG_QSPI_0_CMD = (1 << 2) | (cs & 0x030000); // 32-bit write with CS
    
    // Write data
    REG_QSPI_0_TXFIFO = data;
    
    return STATUS_SUCCESS;
}

static status_t qspi_dma_write_impl(const uint32_t* data, size_t count)
{
    CHECK_NULL(data);
    
    if (count == 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Simulate DMA write by writing each 32-bit word
    for (size_t i = 0; i < count; i++) {
        qspi_write_32_impl(data[i]);
    }
    
    return STATUS_SUCCESS;
}

static status_t qspi_dma_read_impl(uint32_t* data, size_t count)
{
    CHECK_NULL(data);
    
    if (count == 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Simulate DMA read by reading each 32-bit word
    for (size_t i = 0; i < count; i++) {
        qspi_read_32_impl(&data[i]);
    }
    
    return STATUS_SUCCESS;
}

static status_t qspi_is_busy_impl(bool_t* busy)
{
    CHECK_NULL(busy);
    
    if (REG_QSPI_0_STATUS & (1 << 0)) {
        *busy = TRUE;
    } else {
        *busy = FALSE;
    }
    
    return STATUS_SUCCESS;
}