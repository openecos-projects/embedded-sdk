#include "ecos/shell/fs.h"

#include "ff.h"
#include "stdio.h"
#include "string.h"

static Shell *current_shell(void)
{
    Shell *shell = shellGetCurrent();

    return shell != NULL && shellGetPath(shell) != NULL ? shell : NULL;
}

int ecos_shell_fs_init(Shell *shell, char *path, size_t capacity)
{
    if (shell == NULL || path == NULL || capacity < ECOS_SHELL_FS_PATH_CAPACITY)
        return -1;
    if (f_chdir("0:/") != FR_OK ||
        f_getcwd(path, ECOS_SHELL_FS_PATH_CAPACITY) != FR_OK)
        return -1;

    shellSetPath(shell, path);
    return 0;
}

int ecos_shell_fs_cd(int argc, char *argv[])
{
    Shell *shell = current_shell();
    char previous[ECOS_SHELL_FS_PATH_CAPACITY];
    char *path;
    FRESULT result;
    size_t length;

    if (shell == NULL)
        return -1;
    if (argc != 2) {
        shellWriteString(shell, "Usage: cd <directory>\r\n");
        return -1;
    }

    path = shellGetPath(shell);
    length = strlen(path);
    if (length >= sizeof(previous))
        return -1;
    memcpy(previous, path, length + 1u);

    result = f_chdir(argv[1]);
    if (result == FR_OK) {
        result = f_getcwd(path, ECOS_SHELL_FS_PATH_CAPACITY);
        if (result == FR_OK)
            return 0;
        (void)f_chdir(previous);
        memcpy(path, previous, length + 1u);
    }

    shellPrint(shell, "cd: failed (code %d)\r\n", (int)result);
    return -1;
}

int ecos_shell_fs_ls(int argc, char *argv[])
{
    Shell *shell = current_shell();
    DIR directory;
    FILINFO entry;
    char line[320];
    const char *path;
    FRESULT result;
    FRESULT close_result;

    if (shell == NULL)
        return -1;
    if (argc > 2) {
        shellWriteString(shell, "Usage: ls [directory]\r\n");
        return -1;
    }

    path = argc == 2 ? argv[1] : shellGetPath(shell);
    result = f_opendir(&directory, path);
    if (result != FR_OK) {
        shellPrint(shell, "ls: cannot open directory (code %d)\r\n", (int)result);
        return -1;
    }

    for (;;) {
        int length;

        result = f_readdir(&directory, &entry);
        if (result != FR_OK || entry.fname[0] == '\0')
            break;

        if (entry.fattrib & AM_DIR) {
            length = snprintf(line, sizeof(line), "[DIR]  %s\r\n", entry.fname);
        } else {
            length = snprintf(line, sizeof(line), "[FILE] %s  (%lu bytes)\r\n",
                              entry.fname, (unsigned long)entry.fsize);
        }
        if (length < 0 || (size_t)length >= sizeof(line)) {
            result = FR_NOT_ENOUGH_CORE;
            break;
        }
        shellWriteString(shell, line);
    }

    close_result = f_closedir(&directory);
    if (result == FR_OK && close_result == FR_OK)
        return 0;
    shellPrint(shell, "ls: read failed (code %d)\r\n",
               (int)(result != FR_OK ? result : close_result));
    return -1;
}

int ecos_shell_fs_touch(int argc, char *argv[])
{
    Shell *shell = current_shell();
    FIL file;
    FRESULT result;

    if (shell == NULL)
        return -1;
    if (argc != 2) {
        shellWriteString(shell, "Usage: touch <filename>\r\n");
        return -1;
    }

    result = f_open(&file, argv[1], FA_OPEN_ALWAYS | FA_WRITE);
    if (result == FR_OK)
        result = f_close(&file);
    if (result == FR_OK)
        return 0;

    shellPrint(shell, "touch: failed (code %d)\r\n", (int)result);
    return -1;
}
