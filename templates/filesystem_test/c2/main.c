#include "main.h"
#include "ffinit.h"


void main(void){
    
    hal_sys_uart_init();
    hal_sys_putstr("Hello, World!\n\r");

    load_filesystem();


    while(1){
        // printf("ENDING\n");
    }
}
