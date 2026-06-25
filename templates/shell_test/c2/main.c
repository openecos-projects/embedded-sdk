#include "main.h"

void easy_print(void* param){
    printf("I will print: %s\r\n", (char*)param);
}


// Warn: 若使用fatfs启动shell，需要额外注意用户程序的栈分配：
//       fatfs分配FIL file时容易导致栈溢出，进而导致FatFs结构体损坏，最终无法使用f_open()打开文件

void main(){
    hal_sys_uart_init();

    // 为exec_func功能创建函数调用链
    flist[0] = (func_node){
        .func = easy_print,
        .default_param = "Hello, World",
    };

    create_shell_env_varible();

    load_shell();

    while(1){
        printf("Never Reaching here.\r\n");
    }
    
}
