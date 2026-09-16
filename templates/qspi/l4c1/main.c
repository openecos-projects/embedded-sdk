#include "main.h"
#include "hal_sys_uart.h"
#include "hal_qspi.h"
#include "ecos/log.h"

void main(void){
    hal_sys_uart_init();
    (void)ecos_log_set_level(ECOS_LOG_DEBUG);
    ECOS_LOGI("qspi", "Starting qspi test...");

    hal_qspi_config_t qspi_config = {
        .clkdiv = 4,
    };
    hal_qspi_init(HAL_QSPI_PORT_0, &qspi_config);
    ECOS_LOGI("qspi", "QSPI initialized.");

    ECOS_LOGI("qspi", "qspi test completed successfully.");
    while(1);
}
