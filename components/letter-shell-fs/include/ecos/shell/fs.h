#ifndef ECOS_SHELL_FS_H
#define ECOS_SHELL_FS_H

#include "shell.h"

#include <stddef.h>

#define ECOS_SHELL_FS_PATH_CAPACITY 512u

int ecos_shell_fs_init(Shell *shell, char *path, size_t capacity);
int ecos_shell_fs_cd(int argc, char *argv[]);
int ecos_shell_fs_ls(int argc, char *argv[]);
int ecos_shell_fs_touch(int argc, char *argv[]);

#endif
