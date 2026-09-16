#include "ecos/device/esp01s_at.h"

#include "ecos/driver/timer.h"

#include <stddef.h>

#define ESP01S_TRY(expression)                   \
    do {                                         \
        ecos_err_t esp01s_result = (expression); \
        if (esp01s_result != ECOS_OK)            \
            return esp01s_result;                \
    } while (0)

#define ESP01S_RESPONSE_SIZE 256u
#define ESP01S_COMMAND_SIZE 160u
#define ESP01S_SHORT_TIMEOUT_MS 2000u
#define ESP01S_JOIN_TIMEOUT_MS 20000u
#define ESP01S_POLL_INTERVAL_MS 1u

typedef enum {
    ESP01S_EXPECT_OK = 0,
    ESP01S_EXPECT_PROMPT
} ecos_esp01s_expect_t;

/* The runtime links against no libc, so strings are handled by hand. */
static size_t esp01s_length(const char *text)
{
    size_t length = 0u;

    while (text[length] != '\0')
        ++length;
    return length;
}

static void esp01s_copy(char *destination, const char *source)
{
    while (*source != '\0')
        *destination++ = *source++;
    *destination = '\0';
}

/* Returns 1 when haystack contains needle, comparing at most
 * haystack_length bytes. */
static int esp01s_contains(const char *haystack,
                           size_t haystack_length,
                           const char *needle)
{
    size_t needle_length = esp01s_length(needle);
    size_t index;

    if (needle_length == 0u || haystack_length < needle_length)
        return 0;
    for (index = 0u; index + needle_length <= haystack_length; ++index) {
        size_t offset = 0u;

        while (offset < needle_length &&
               haystack[index + offset] == needle[offset])
            ++offset;
        if (offset == needle_length)
            return 1;
    }
    return 0;
}

static ecos_err_t esp01s_write_string(ecos_esp01s_t *module, const char *text)
{
    int result = ecos_uart_write(
        module->config.uart, text, esp01s_length(text)
    );

    return result < 0 ? (ecos_err_t)result : ECOS_OK;
}

/* Collects the response until the expected terminator appears or the
 * deadline passes. The accumulated bytes are NUL-terminated in response
 * when response is not NULL. */
static ecos_err_t esp01s_await(ecos_esp01s_t *module,
                               ecos_esp01s_expect_t expect,
                               uint32_t timeout_ms,
                               char *response,
                               size_t response_size)
{
    size_t length = 0u;
    uint32_t elapsed = 0u;

    for (;;) {
        uint8_t byte;
        int read_result = ecos_uart_try_read(module->config.uart, &byte);

        if (read_result < 0)
            return (ecos_err_t)read_result;
        if (read_result == 1 && length < ESP01S_RESPONSE_SIZE - 1u) {
            response[length++] = (char)byte;
            if (esp01s_contains(response, length, "ERROR"))
                return ECOS_ERR_IO;
            if (expect == ESP01S_EXPECT_OK &&
                esp01s_contains(response, length, "OK"))
                break;
            if (expect == ESP01S_EXPECT_PROMPT &&
                esp01s_contains(response, length, ">"))
                break;
        }
        if (read_result == 0) {
            if (elapsed >= timeout_ms)
                return ECOS_ERR_TIMEOUT;
            ESP01S_TRY(ecos_timer_delay_ms(ECOS_TIMER_DEFAULT,
                                           ESP01S_POLL_INTERVAL_MS));
            elapsed += ESP01S_POLL_INTERVAL_MS;
        }
    }
    response[length] = '\0';
    return ECOS_OK;
}

/* Sends one AT command line (terminated with CRLF) and waits for OK. */
static ecos_err_t esp01s_command(ecos_esp01s_t *module,
                                 const char *command,
                                 uint32_t timeout_ms,
                                 char *response,
                                 size_t response_size)
{
    ESP01S_TRY(esp01s_write_string(module, command));
    ESP01S_TRY(esp01s_write_string(module, "\r\n"));
    return esp01s_await(module, ESP01S_EXPECT_OK, timeout_ms,
                        response, response_size);
}

static ecos_err_t esp01s_check_ready(const ecos_esp01s_t *module)
{
    if (module == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (module->initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    return ECOS_OK;
}

ecos_err_t ecos_esp01s_init(ecos_esp01s_t *module,
                            const ecos_esp01s_config_t *config)
{
    char response[ESP01S_RESPONSE_SIZE];
    ecos_uart_config_t uart_config = ECOS_UART_CONFIG_DEFAULT;

    if (module == NULL || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    module->config = *config;
    module->initialized = 0u;

    uart_config.baud_rate = config->baud_rate;
    ESP01S_TRY(ecos_uart_init(config->uart, &uart_config));

    module->initialized = 1u;
    ESP01S_TRY(esp01s_command(module, "AT", ESP01S_SHORT_TIMEOUT_MS,
                              response, sizeof(response)));
    ESP01S_TRY(esp01s_command(module, "ATE0", ESP01S_SHORT_TIMEOUT_MS,
                              response, sizeof(response)));
    return ECOS_OK;
}

ecos_err_t ecos_esp01s_deinit(ecos_esp01s_t *module)
{
    if (module == NULL || module->initialized == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    module->initialized = 0u;
    return ECOS_OK;
}

ecos_err_t ecos_esp01s_get_version(ecos_esp01s_t *module,
                                   char *version,
                                   size_t size)
{
    char response[ESP01S_RESPONSE_SIZE];
    size_t length;

    ESP01S_TRY(esp01s_check_ready(module));
    if (version == NULL || size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    ESP01S_TRY(esp01s_command(module, "AT+GMR", ESP01S_SHORT_TIMEOUT_MS,
                              response, sizeof(response)));
    length = esp01s_length(response);
    if (length >= size)
        length = size - 1u;
    for (size_t index = 0u; index < length; ++index)
        version[index] = response[index];
    version[length] = '\0';
    return ECOS_OK;
}

ecos_err_t ecos_esp01s_set_mode(ecos_esp01s_t *module,
                                ecos_esp01s_mode_t mode)
{
    char command[] = "AT+CWMODE=0";
    char response[ESP01S_RESPONSE_SIZE];

    ESP01S_TRY(esp01s_check_ready(module));
    if (mode < ECOS_ESP01S_MODE_STA || mode > ECOS_ESP01S_MODE_AP_STA)
        return ECOS_ERR_INVALID_ARGUMENT;

    command[sizeof(command) - 2u] = (char)('0' + (int)mode);
    return esp01s_command(module, command, ESP01S_SHORT_TIMEOUT_MS,
                          response, sizeof(response));
}

ecos_err_t ecos_esp01s_set_connection_mode(
    ecos_esp01s_t *module, ecos_esp01s_connection_mode_t mode)
{
    char command[] = "AT+CIPMUX=0";
    char response[ESP01S_RESPONSE_SIZE];

    ESP01S_TRY(esp01s_check_ready(module));
    if (mode != ECOS_ESP01S_CONNECTION_SINGLE &&
        mode != ECOS_ESP01S_CONNECTION_MULTI)
        return ECOS_ERR_INVALID_ARGUMENT;

    command[sizeof(command) - 2u] = (char)('0' + (int)mode);
    return esp01s_command(module, command, ESP01S_SHORT_TIMEOUT_MS,
                          response, sizeof(response));
}

ecos_err_t ecos_esp01s_wifi_join(ecos_esp01s_t *module,
                                 const char *ssid,
                                 const char *password)
{
    char command[ESP01S_COMMAND_SIZE];
    char response[ESP01S_RESPONSE_SIZE];
    char *cursor = command;
    char *end = command + sizeof(command) - 1u;

    ESP01S_TRY(esp01s_check_ready(module));
    if (ssid == NULL || password == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* AT+CWJAP="<ssid>","<password>" */
    const char *segments[] = { "AT+CWJAP=\"", ssid, "\",\"", password, "\"" };
    for (size_t segment = 0u; segment < 5u; ++segment) {
        const char *text = segments[segment];

        while (*text != '\0') {
            if (cursor >= end)
                return ECOS_ERR_INVALID_ARGUMENT;
            *cursor++ = *text++;
        }
    }
    *cursor = '\0';

    return esp01s_command(module, command, ESP01S_JOIN_TIMEOUT_MS,
                          response, sizeof(response));
}

ecos_err_t ecos_esp01s_tcp_connect(ecos_esp01s_t *module,
                                   const char *host,
                                   const char *port)
{
    char command[ESP01S_COMMAND_SIZE];
    char response[ESP01S_RESPONSE_SIZE];
    char *cursor = command;
    char *end = command + sizeof(command) - 1u;

    ESP01S_TRY(esp01s_check_ready(module));
    if (host == NULL || port == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* AT+CIPSTART="TCP","<host>",<port> */
    const char *segments[] = { "AT+CIPSTART=\"TCP\",\"", host, "\",", port };
    for (size_t segment = 0u; segment < 4u; ++segment) {
        const char *text = segments[segment];

        while (*text != '\0') {
            if (cursor >= end)
                return ECOS_ERR_INVALID_ARGUMENT;
            *cursor++ = *text++;
        }
    }
    *cursor = '\0';

    return esp01s_command(module, command, ESP01S_JOIN_TIMEOUT_MS,
                          response, sizeof(response));
}

ecos_err_t ecos_esp01s_enter_passthrough(ecos_esp01s_t *module)
{
    char response[ESP01S_RESPONSE_SIZE];

    ESP01S_TRY(esp01s_check_ready(module));
    ESP01S_TRY(esp01s_command(module, "AT+CIPMODE=1",
                              ESP01S_SHORT_TIMEOUT_MS,
                              response, sizeof(response)));
    ESP01S_TRY(esp01s_write_string(module, "AT+CIPSEND\r\n"));
    return esp01s_await(module, ESP01S_EXPECT_PROMPT,
                        ESP01S_SHORT_TIMEOUT_MS,
                        response, sizeof(response));
}

ecos_err_t ecos_esp01s_send(ecos_esp01s_t *module,
                            const void *data,
                            size_t size)
{
    int result;

    ESP01S_TRY(esp01s_check_ready(module));
    if (data == NULL && size != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    result = ecos_uart_write(module->config.uart, data, size);
    return result < 0 ? (ecos_err_t)result : ECOS_OK;
}
