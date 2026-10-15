#ifndef __SHELL_INIT_H__
#define __SHELL_INIT_H__

#include "shell.h"
#include "shell_fs.h"
#include "shell_port.h"
#include "ecos/bsp/console.h"

#ifdef CONFIG_COMPONENT_FLASH_FS
#include "ffinit.h"
#endif

void load_shell();

#endif
