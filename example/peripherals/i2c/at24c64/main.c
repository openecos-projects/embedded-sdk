#include "ecos/bsp/console.h"
#include "ecos/device/at24c64.h"
#include "ecos/driver/i2c.h"
#include "ecos/log.h"

#include <stddef.h>
#include <stdint.h>

#define LOG_TAG "i2c-at24c64"
/* Crosses one 32-byte page boundary to exercise paged writes. */
#define TEST_BASE_ADDRESS 0x0010u
#define TEST_DATA_SIZE 40u

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

static char hex_digit(unsigned value)
{
    return (char)(value < 10u ? '0' + value : 'a' + (value - 10u));
}

static void console_puts(const char *text)
{
    size_t length = 0u;

    while (text[length] != '\0')
        ++length;
    (void)bsp_console_write(text, length);
}

static void print_hex_dump(const char *title,
                           uint16_t base,
                           const uint8_t *data,
                           size_t size)
{
    char line[5u + 1u + 16u * 3u + 2u];
    size_t offset;

    console_puts(title);
    for (offset = 0u; offset < size; offset += 16u) {
        size_t cursor = 0u;
        size_t column;
        uint16_t address = (uint16_t)(base + offset);

        line[cursor++] = hex_digit(address >> 12);
        line[cursor++] = hex_digit((address >> 8) & 0x0Fu);
        line[cursor++] = hex_digit((address >> 4) & 0x0Fu);
        line[cursor++] = hex_digit(address & 0x0Fu);
        line[cursor++] = ':';
        for (column = 0u; column < 16u; ++column) {
            size_t index = offset + column;

            line[cursor++] = ' ';
            if (index < size) {
                line[cursor++] = hex_digit(data[index] >> 4);
                line[cursor++] = hex_digit(data[index] & 0x0Fu);
            } else {
                line[cursor++] = ' ';
                line[cursor++] = ' ';
            }
        }
        line[cursor++] = '\n';
        (void)bsp_console_write(line, cursor);
    }
}

int main(void)
{
    const ecos_i2c_config_t i2c_config = ECOS_I2C_CONFIG_DEFAULT;
    const ecos_at24c64_config_t eeprom_config = ECOS_AT24C64_CONFIG_DEFAULT;
    ecos_at24c64_t eeprom;
    uint8_t written[TEST_DATA_SIZE];
    uint8_t readback[TEST_DATA_SIZE];
    size_t index;
    unsigned mismatch_count = 0u;

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_init(ECOS_I2C_DEFAULT, &i2c_config),
        "initialize I2C controller"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_at24c64_init(&eeprom, &eeprom_config),
        "initialize AT24C64"
    );

    for (index = 0u; index < TEST_DATA_SIZE; ++index)
        written[index] = (uint8_t)(0xA0u + index);

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_at24c64_write(
            &eeprom, TEST_BASE_ADDRESS, written, sizeof(written)
        ),
        "write AT24C64"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_at24c64_read(
            &eeprom, TEST_BASE_ADDRESS, readback, sizeof(readback)
        ),
        "read AT24C64"
    );

    for (index = 0u; index < TEST_DATA_SIZE; ++index) {
        if (readback[index] != written[index])
            ++mismatch_count;
    }

    print_hex_dump("written:\n", TEST_BASE_ADDRESS, written, sizeof(written));
    print_hex_dump("readback:\n", TEST_BASE_ADDRESS, readback, sizeof(readback));

    if (mismatch_count == 0u) {
        (void)ECOS_LOGI(LOG_TAG, "PASS: %u bytes verified", TEST_DATA_SIZE);
    } else {
        (void)ECOS_LOGE(
            LOG_TAG, "FAIL: %u byte(s) mismatch", mismatch_count
        );
    }

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_at24c64_deinit(&eeprom), "deinitialize AT24C64"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_i2c_deinit(ECOS_I2C_DEFAULT),
        "deinitialize I2C controller"
    );
    halt();
}
