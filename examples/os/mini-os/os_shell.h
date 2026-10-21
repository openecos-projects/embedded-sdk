#ifndef MINI_OS_SHELL_H
#define MINI_OS_SHELL_H

#include "kernel.h"

/* Initialise the Letter-Shell instance and install the kernel commands. */
int os_shell_init(void);

/* Cooperative shell task: polls the console and feeds the parser. */
void os_shell_entry(task_t *task);

#endif
