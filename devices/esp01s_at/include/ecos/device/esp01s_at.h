#ifndef ECOS_DEVICE_ESP01S_AT_H
#define ECOS_DEVICE_ESP01S_AT_H

#include "ecos/driver/uart.h"
#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_ESP01S_MODE_STA = 1,
    ECOS_ESP01S_MODE_AP = 2,
    ECOS_ESP01S_MODE_AP_STA = 3
} ecos_esp01s_mode_t;

typedef enum {
    ECOS_ESP01S_CONNECTION_SINGLE = 0,
    ECOS_ESP01S_CONNECTION_MULTI = 1
} ecos_esp01s_connection_mode_t;

typedef struct {
    ecos_uart_port_t uart;
    uint32_t baud_rate;
} ecos_esp01s_config_t;

typedef struct {
    ecos_esp01s_config_t config;
    uint8_t initialized;
} ecos_esp01s_t;

/* ESP01S is wired to the UART1 (hp block) pins on StarrySky L4 boards. */
#define ECOS_ESP01S_CONFIG_DEFAULT \
    { ECOS_UART_PORT_1, 115200u }

/* Initializes the UART port, verifies the AT handshake and turns the
 * command echo off. */
ecos_err_t ecos_esp01s_init(ecos_esp01s_t *module,
                            const ecos_esp01s_config_t *config);
ecos_err_t ecos_esp01s_deinit(ecos_esp01s_t *module);

/* Queries the AT firmware version (AT+GMR); the first response line is
 * copied to version (NUL-terminated, truncated to size - 1). */
ecos_err_t ecos_esp01s_get_version(ecos_esp01s_t *module,
                                   char *version,
                                   size_t size);

ecos_err_t ecos_esp01s_set_mode(ecos_esp01s_t *module,
                                ecos_esp01s_mode_t mode);
ecos_err_t ecos_esp01s_set_connection_mode(
    ecos_esp01s_t *module, ecos_esp01s_connection_mode_t mode);

/* Joins a WPA/WPA2 access point; takes several seconds on air. */
ecos_err_t ecos_esp01s_wifi_join(ecos_esp01s_t *module,
                                 const char *ssid,
                                 const char *password);

/* Opens a single TCP connection (CIPMUX=0 must be selected first). */
ecos_err_t ecos_esp01s_tcp_connect(ecos_esp01s_t *module,
                                   const char *host,
                                   const char *port);

/* Enters transparent transmission (CIPMODE=1 + CIPSEND, waits for '>'). */
ecos_err_t ecos_esp01s_enter_passthrough(ecos_esp01s_t *module);

/* Writes raw bytes while in transparent transmission mode. */
ecos_err_t ecos_esp01s_send(ecos_esp01s_t *module,
                            const void *data,
                            size_t size);

#ifdef __cplusplus
}
#endif

#endif
