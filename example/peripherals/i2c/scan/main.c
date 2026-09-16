#include "ecos/bsp/console.h"
#include "ecos/driver/i2c.h"
#include "ecos/log.h"

#include <stddef.h>
#include <stdint.h>

#define I2C_SCAN_CONTROLLER ECOS_I2C_DEFAULT
#define I2C_SCAN_FIRST_ADDRESS 0x08u
#define I2C_SCAN_LAST_ADDRESS 0x77u
#define I2C_SCAN_ADDRESS_SPACE 0x80u
#define LOG_TAG "i2c-scan"

/* BSS is zeroed by the boot loader; a stack array would pull in libc memset. */
static uint8_t g_found[I2C_SCAN_ADDRESS_SPACE];

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

static void console_puts(const char *text)
{
    size_t length = 0u;

    while (text[length] != '\0')
        ++length;
    (void)bsp_console_write(text, length);
}

static char hex_digit(unsigned value)
{
    return (char)(value < 10u ? '0' + value : 'a' + (value - 10u));
}

/* Print the probe bitmap as an i2cdetect-style address map: rows are the
 * high address nibble, columns the low nibble; ACKed addresses show their
 * hex value, everything else shows "--". */
static void print_address_map(const uint8_t *found)
{
    char line[4u + 16u * 3u + 2u];
    unsigned row;
    unsigned column;
    size_t cursor;

    console_puts("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (row = 0u; row < I2C_SCAN_ADDRESS_SPACE / 16u; ++row) {
        line[0] = hex_digit(row);
        line[1] = '0';
        line[2] = ':';
        line[3] = ' ';
        cursor = 4u;
        for (column = 0u; column < 16u; ++column) {
            unsigned address = row * 16u + column;

            if (address >= I2C_SCAN_FIRST_ADDRESS &&
                address <= I2C_SCAN_LAST_ADDRESS && found[address] != 0u) {
                line[cursor] = hex_digit(address >> 4);
                line[cursor + 1u] = hex_digit(address & 0x0Fu);
            } else {
                line[cursor] = '-';
                line[cursor + 1u] = '-';
            }
            line[cursor + 2u] = ' ';
            cursor += 3u;
        }
        line[cursor] = '\n';
        (void)bsp_console_write(line, cursor + 1u);
    }
}

int main(void)
{
    const ecos_i2c_config_t config = ECOS_I2C_CONFIG_DEFAULT;
    unsigned address;
    unsigned device_count = 0u;
    int instance_count;
    int probe_result;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );

    instance_count = ecos_i2c_get_instance_count();
    ECOS_PANIC_ON_ERROR(LOG_TAG, instance_count, "query I2C controllers");
    if (instance_count <= (int)I2C_SCAN_CONTROLLER)
        ECOS_PANIC_ON_ERROR(
            LOG_TAG, ECOS_ERR_NOT_FOUND, "find default I2C controller"
        );

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_init(I2C_SCAN_CONTROLLER, &config),
        "initialize I2C controller"
    );

    (void)ECOS_LOGI(
        LOG_TAG,
        "Scanning 7-bit I2C addresses 0x%02X-0x%02X",
        I2C_SCAN_FIRST_ADDRESS,
        I2C_SCAN_LAST_ADDRESS
    );

    for (address = I2C_SCAN_FIRST_ADDRESS;
         address <= I2C_SCAN_LAST_ADDRESS; ++address) {
        probe_result = ecos_i2c_probe(
            I2C_SCAN_CONTROLLER, (uint8_t)address
        );
        if (ecos_result_failed(probe_result)) {
            (void)ecos_i2c_deinit(I2C_SCAN_CONTROLLER);
            ECOS_PANIC_ON_ERROR(
                LOG_TAG, probe_result, "probe I2C address"
            );
        }
        if (probe_result == 1) {
            ++device_count;
            g_found[address] = 1u;
        }
    }

    print_address_map(g_found);

    (void)ECOS_LOGI(
        LOG_TAG, "Scan complete: %u device(s) found", device_count
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_deinit(I2C_SCAN_CONTROLLER),
        "deinitialize I2C controller"
    );
    halt();
}
