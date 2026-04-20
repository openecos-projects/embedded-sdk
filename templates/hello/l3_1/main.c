#include "main.h"

void main(void){
    
    hal_sys_uart_init();
    hal_sys_putstr("Hello, World!\n\r");

}