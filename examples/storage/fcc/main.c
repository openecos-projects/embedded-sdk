#include "ecos/bsp/console.h"
#include "ecos/log.h"
#include "ecos/shell/fs.h"
#include "fcc.h"
#include "ffinit.h"
#include "shell.h"
#include "shell_port.h"

#include "string.h"

#define FCC_COMMAND_CAPACITY 40u

static char shell_buffer[1024];
static char shell_path[ECOS_SHELL_FS_PATH_CAPACITY];
static Shell shell;
static ShellCommand commands[FCC_COMMAND_CAPACITY];
static const ShellCommand file_commands[] = {
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0) |
                       SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                       SHELL_CMD_DISABLE_RETURN,
                   cd, ecos_shell_fs_cd, change directory),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0) |
                       SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                       SHELL_CMD_DISABLE_RETURN,
                   ls, ecos_shell_fs_ls, list files),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0) |
                       SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                       SHELL_CMD_DISABLE_RETURN,
                   touch, ecos_shell_fs_touch, create file),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0) |
                       SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                       SHELL_CMD_DISABLE_RETURN,
                   fcc, fccCmd, compile C source),
    SHELL_CMD_ITEM(SHELL_CMD_PERMISSION(0) |
                       SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                       SHELL_CMD_DISABLE_RETURN,
                   fcc-run, fccRunCmd, run compiled main),
};

int main(void)
{
    unsigned short count;

    ECOS_PANIC_ON_ERROR("fcc", bsp_console_init(), "initialize console");
    if (!load_filesystem())
        return 1;
    if (ecos_shell_fs_init(&shell, shell_path, sizeof(shell_path)) != 0) {
        (void)ECOS_LOGE("fcc", "initialize file commands failed");
        return 1;
    }

    shell.read = shellRead;
    shell.write = shellWrite;
    shellInit(&shell, shell_buffer, sizeof(shell_buffer));

    count = shell.commandList.count;
    if ((size_t)count + sizeof(file_commands) / sizeof(file_commands[0]) >
        FCC_COMMAND_CAPACITY) {
        (void)ECOS_LOGE("fcc", "shell command table is full");
        shellRemove(&shell);
        return 1;
    }
    memcpy(commands, shell.commandList.base, count * sizeof(commands[0]));
    memcpy(commands + count, file_commands, sizeof(file_commands));
    shell.commandList.base = commands;
    shell.commandList.count = count +
        (unsigned short)(sizeof(file_commands) / sizeof(file_commands[0]));

    shellTask(&shell);
    return 0;
}
