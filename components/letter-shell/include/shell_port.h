#ifndef __SHELL_PORT_H__
#define __SHELL_PORT_H__

#include "stdio.h"
#include "log.h"
#include "stdint.h"
#include "string.h"
#include "generated/autoconf.h"


extern uint32_t envInt;
extern uint16_t envShort;
extern char envString[128];
extern char envChar;
extern uint32_t envFunc;

typedef struct func_node{
    void (*func)(void *);
    void* default_param;
} func_node;

extern func_node flist[16];

void create_shell_env_varible();

short shellRead(char* str, unsigned short len);

short shellWrite(char* str, unsigned short len);

#ifdef CONFIG_COMPONENT_FLASH_FS

#include "ff.h"

size_t getcwd(char* dir, size_t dirLen);

size_t chdir(char * dir);

// 将dir的内容输出到buffer中
size_t listdir(char *dir, char *buffer, size_t maxLen);

size_t createfile(char *dir, char *filename);

#endif

#endif
