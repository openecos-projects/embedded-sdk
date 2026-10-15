#include "ecos/shell/fs.h"

#include "ff.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static Shell shell;
static char current_directory[32] = "/";
static char output[1024];
static char opened_path[64];
static char listed_path[64];
static BYTE opened_mode;
static unsigned int entry_index;

Shell *shellGetCurrent(void)
{
    return &shell;
}

unsigned short shellWriteString(Shell *active, const char *text)
{
    size_t length = strlen(text);
    size_t used = strlen(output);

    assert(active == &shell);
    assert(used + length < sizeof(output));
    memcpy(output + used, text, length + 1u);
    return (unsigned short)length;
}

void shellPrint(Shell *active, const char *format, ...)
{
    va_list args;
    size_t used = strlen(output);

    assert(active == &shell);
    va_start(args, format);
    assert(vsnprintf(output + used, sizeof(output) - used, format, args) >= 0);
    va_end(args);
}

FRESULT f_chdir(const TCHAR *path)
{
    if (strcmp(path, "0:/") == 0 || strcmp(path, "/") == 0) {
        strcpy(current_directory, "/");
    } else if (strcmp(path, "sub") == 0 || strcmp(path, "/sub") == 0) {
        strcpy(current_directory, "/sub");
    } else if (strcmp(path, "big") == 0) {
        strcpy(current_directory, "/big");
    } else {
        return FR_NO_PATH;
    }
    return FR_OK;
}

FRESULT f_getcwd(TCHAR *path, UINT capacity)
{
    if (strcmp(current_directory, "/big") == 0) {
        strcpy(path, "partial");
        return FR_NOT_ENOUGH_CORE;
    }
    assert(capacity == ECOS_SHELL_FS_PATH_CAPACITY);
    strcpy(path, current_directory);
    return FR_OK;
}

FRESULT f_opendir(DIR *directory, const TCHAR *path)
{
    (void)directory;
    strcpy(listed_path, path);
    entry_index = 0u;
    return FR_OK;
}

FRESULT f_readdir(DIR *directory, FILINFO *entry)
{
    (void)directory;
    memset(entry, 0, sizeof(*entry));
    if (entry_index++ == 0u) {
        strcpy(entry->fname, "docs");
        entry->fattrib = AM_DIR;
    } else if (entry_index == 2u) {
        strcpy(entry->fname, "notes.txt");
        entry->fsize = 17u;
    }
    return FR_OK;
}

FRESULT f_closedir(DIR *directory)
{
    (void)directory;
    return FR_OK;
}

FRESULT f_open(FIL *file, const TCHAR *path, BYTE mode)
{
    (void)file;
    strcpy(opened_path, path);
    opened_mode = mode;
    return FR_OK;
}

FRESULT f_close(FIL *file)
{
    (void)file;
    return FR_OK;
}

int main(void)
{
    char path[ECOS_SHELL_FS_PATH_CAPACITY];
    char *cd_sub[] = {"cd", "sub"};
    char *cd_big[] = {"cd", "big"};
    char *ls[] = {"ls"};
    char *touch[] = {"touch", "notes.txt"};

    assert(ecos_shell_fs_init(&shell, path, sizeof(path)) == 0);
    assert(shellGetPath(&shell) == path);
    assert(strcmp(path, "/") == 0);

    assert(ecos_shell_fs_cd(2, cd_sub) == 0);
    assert(strcmp(path, "/sub") == 0);
    assert(ecos_shell_fs_ls(1, ls) == 0);
    assert(strcmp(listed_path, "/sub") == 0);
    assert(strstr(output, "[DIR]  docs") != NULL);
    assert(strstr(output, "[FILE] notes.txt  (17 bytes)") != NULL);

    assert(ecos_shell_fs_touch(2, touch) == 0);
    assert(strcmp(opened_path, "notes.txt") == 0);
    assert(opened_mode == (FA_OPEN_ALWAYS | FA_WRITE));
    assert((opened_mode & FA_CREATE_ALWAYS) == 0u);

    assert(ecos_shell_fs_cd(2, cd_big) == -1);
    assert(strcmp(path, "/sub") == 0);
    assert(strcmp(current_directory, "/sub") == 0);
    return 0;
}
