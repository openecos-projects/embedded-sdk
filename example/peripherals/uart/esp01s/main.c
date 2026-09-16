#include "ecos/bsp/console.h"
#include "ecos/device/esp01s_at.h"
#include "ecos/log.h"

#include <stddef.h>

#define LOG_TAG "uart-esp01s"

/* Fill in the access point credentials to exercise ecos_esp01s_wifi_join;
 * when left empty the example stops after the AT handshake and mode setup. */
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

static size_t text_length(const char *text)
{
    size_t length = 0u;

    while (text[length] != '\0')
        ++length;
    return length;
}

static void console_write_line(const char *text)
{
    (void)bsp_console_write(text, text_length(text));
    (void)bsp_console_write("\n", 1u);
}

int main(void)
{
    const ecos_esp01s_config_t module_config = ECOS_ESP01S_CONFIG_DEFAULT;
    ecos_esp01s_t module;
    char version[128];

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_esp01s_init(&module, &module_config),
        "initialize ESP01S (AT handshake)"
    );
    (void)ECOS_LOGI(LOG_TAG, "AT handshake OK");

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_esp01s_get_version(&module, version, sizeof(version)),
        "query AT firmware version"
    );
    (void)ECOS_LOGI(LOG_TAG, "firmware version:");
    console_write_line(version);

    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_esp01s_set_mode(&module, ECOS_ESP01S_MODE_STA),
        "select station mode"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG,
        ecos_esp01s_set_connection_mode(
            &module, ECOS_ESP01S_CONNECTION_SINGLE
        ),
        "select single connection mode"
    );
    (void)ECOS_LOGI(LOG_TAG, "station mode, single connection");

    if (text_length(WIFI_SSID) == 0u) {
        (void)ECOS_LOGI(
            LOG_TAG,
            "WIFI_SSID is empty, skipping wifi_join; "
            "edit main.c to join an access point"
        );
    } else {
        ECOS_PANIC_ON_ERROR(
            LOG_TAG,
            ecos_esp01s_wifi_join(&module, WIFI_SSID, WIFI_PASSWORD),
            "join access point"
        );
        (void)ECOS_LOGI(LOG_TAG, "joined access point %s", WIFI_SSID);

        /* Extend here, for example:
         *   ecos_esp01s_tcp_connect(&module, "192.168.1.10", "8080");
         *   ecos_esp01s_enter_passthrough(&module);
         *   ecos_esp01s_send(&module, "hello\n", 6u);
         */
    }

    ECOS_PANIC_ON_ERROR(
        LOG_TAG, ecos_esp01s_deinit(&module), "deinitialize ESP01S"
    );
    (void)ECOS_LOGI(LOG_TAG, "done");
    halt();
}
