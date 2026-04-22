#include "main.h"
#include "hal_sys_uart.h"
#include "hal_qspi.h"
#include "log.h"

void main(void){
    hal_sys_uart_init();
    log_init(LOG_DEBUG, "qspi_test");
    log_info("Starting qspi test...");
    
    hal_qspi_config_t qspi_config = {
        .clkdiv = 4,
    };
    hal_qspi_init(HAL_QSPI_PORT_0, &qspi_config);
    log_info("QSPI initialized.");
    
    log_info("qspi test completed successfully.");
    while(1);
}
