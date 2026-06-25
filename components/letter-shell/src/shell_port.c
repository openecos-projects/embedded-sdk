#include "shell_port.h"
#include "assert.h"

uint32_t envInt;
uint16_t envShort;
char envString[128];
char envChar;
uint32_t envFunc;
func_node flist[16];


void __attribute__((optimize("O0"))) exec_func(uint8_t id, uint32_t param, uint8_t use_default_param){
    if(!(id >= 0 && id <= 15)){
        log_error("The fnode id must between 0 and 15.");
        return;
    }

    if(flist[id].func == NULL){
        log_error("The function is NULL.");
        return;
    }
    

    if(use_default_param) flist[id].func(flist[id].default_param);
    else flist[id].func((void*)param);
}

void create_shell_env_varible(){
    envInt = 20260512;
    envShort = 2026;
    strcpy(envString, "FINALx");
    envChar = 'Y';
    envFunc = (uintptr_t)exec_func;
}


short shellRead(char* str, unsigned short len){
    unsigned short i = 0;
    for (i = 0; i < len; i++) {
        str[i] = (char)hal_sys_getchar();
    }
    return i;
}

short shellWrite(char* str, unsigned short len){
    for(int i=0;i<len;i++) hal_sys_putchar(str[i]);
    return len;
}

#ifdef CONFIG_COMPONENT_FLASH_FS

size_t getcwd(char* dir, size_t dirLen){
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "0:%s%s", dir, "");
    log_info("dir=%s\r\n",dir);
    log_info("full=%s\r\n",fullpath);

    FRESULT res = f_getcwd(fullpath,sizeof(fullpath));
    if (res != FR_OK) log_fatal("f_getcwd failed: %d\r\n", res);
    return res;
}

size_t chdir(char * dir){
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "0:%s%s", dir, "");
    log_info("dir=%s\r\n",dir);
    log_info("full=%s\r\n",fullpath);

    FRESULT res = f_chdir(fullpath);
    if (res != FR_OK) log_info("f_chdir failed: %d\r\n", res);
    return res;
}

// 将dir的内容输出到buffer中
size_t listdir(char *dir, char *buffer, size_t maxLen){
    FRESULT res;
    DIR dir_obj;
    FILINFO fno;
    size_t written = 0;
    int line_len;
    
    // 参数检查
    if (buffer == NULL || maxLen == 0) {
        return 0;
    }
    
    // 确保缓冲区以空字符开头（安全起见）
    buffer[0] = '\0';
    
    // 1. 打开目录
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "0:%s%s", dir, "");
    log_info("dir=%s\r\n",dir);
    log_info("full=%s\r\n",fullpath);
    res = f_opendir(&dir_obj, fullpath);
    if (res != FR_OK) {
        // 打开失败，写入错误信息
        log_fatal("f_opendir failed: %d\r\n", res);
        return strlen(buffer);
    }
    
    // 2. 循环读取目录内容
    for (;;) {
        res = f_readdir(&dir_obj, &fno);
        
        // 读取出错或没有更多文件了
        if (res != FR_OK || fno.fname[0] == '\0') {
            break;
        }
        
        // 检查缓冲区剩余空间（预留结尾'\0'的空间）
        if (written + 80 >= maxLen) {  // 预留80字节给下一行
            snprintf(buffer + written, maxLen - written, "\r\n... (buffer full)\r\n");
            break;
        }
        
        // 3. 根据文件类型格式化输出
        if (fno.fattrib & AM_DIR) {
            // 目录：用 [DIR] 标记
            line_len = snprintf(buffer + written, maxLen - written, 
                               "[DIR]  %s\r\n", fno.fname);
        } else {
            // 文件：显示文件名和大小
            line_len = snprintf(buffer + written, maxLen - written, 
                               "[FILE] %s  (%lu bytes)\r\n", 
                               fno.fname, fno.fsize);
        }
        
        if (line_len > 0) {
            written += line_len;
        }
    }
    
    // 4. 关闭目录
    f_closedir(&dir_obj);
    
    // 确保字符串以'\0'结尾
    buffer[maxLen - 1] = '\0';
    
    return written;
}

size_t createfile(char *dir, char *filename){
    FRESULT res;
    FIL file;
    size_t written = 0;
    
    // 参数检查
    if (filename == NULL || filename[0] == '\0') {
        return 0;
    }
    
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "0:%s%s", dir, filename);
    log_info("dir=%s\r\n",dir);
    log_info("full=%s\r\n",fullpath);
    
    // 创建文件（如果存在则截断，不存在则新建）
    res = f_open(&file, fullpath, FA_CREATE_ALWAYS);
    if (res == FR_OK) f_close(&file);
    else log_fatal("f_open failed: %d\r\n", res);
    
    return written;
}

#endif
