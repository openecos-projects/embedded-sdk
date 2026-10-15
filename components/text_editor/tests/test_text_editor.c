#include "text_editor.h"

#include "ecos/bsp/console.h"
#include "ecos/log.h"
#include "ff.h"
#include "shell.h"

#include <assert.h>
#include <string.h>

static const uint8_t *input_bytes;
static size_t input_size;
static size_t input_index;
static char initial_content[MAX_FILE_SIZE + 1];
static size_t initial_size;
static size_t read_limit = MAX_FILE_SIZE;
static char saved_content[MAX_FILE_SIZE + 1];
static size_t saved_size;
static int file_exists;

static void set_input(const uint8_t *bytes, size_t size)
{
    input_bytes = bytes;
    input_size = size;
    input_index = 0u;
}

Shell *shellGetCurrent(void)
{
    return NULL;
}

int ecos_log_write(ecos_log_level_t level, const char *tag, const char *file,
                   int line, const char *format, ...)
{
    (void)level;
    (void)tag;
    (void)file;
    (void)line;
    (void)format;
    return 0;
}

int bsp_console_write(const char *text, size_t size)
{
    (void)text;
    return (int)size;
}

int bsp_console_read(void *data, size_t size)
{
    if (size != 1u || input_index == input_size)
        return ECOS_ERR_IO;
    *(uint8_t *)data = input_bytes[input_index++];
    return 1;
}

int bsp_console_try_read(uint8_t *data)
{
    if (input_index == input_size)
        return 0;
    *data = input_bytes[input_index++];
    return 1;
}

FRESULT f_open(FIL *file, const TCHAR *path, BYTE mode)
{
    assert(strcmp(path, "0:/notes.txt") == 0);
    if ((mode & FA_READ) != 0u) {
        if (!file_exists) return FR_NO_FILE;
        file->obj.objsize = initial_size;
    } else {
        assert((mode & FA_WRITE) != 0u);
        saved_size = 0u;
    }
    return FR_OK;
}

FRESULT f_read(FIL *file, void *buffer, UINT length, UINT *read_count)
{
    size_t count = initial_size < read_limit ? initial_size : read_limit;

    (void)file;
    assert(length >= initial_size);
    memcpy(buffer, initial_content, count);
    *read_count = (UINT)count;
    return FR_OK;
}

FRESULT f_write(FIL *file, const void *buffer, UINT length, UINT *write_count)
{
    (void)file;
    assert(length <= MAX_FILE_SIZE);
    memcpy(saved_content, buffer, length);
    saved_size = length;
    *write_count = length;
    return FR_OK;
}

FRESULT f_close(FIL *file)
{
    (void)file;
    return FR_OK;
}

int main(void)
{
    static const uint8_t edit_keys[] = {
        'A', '\n', 'B', 0x1bu, '[', 'A', 'C', 0x17u, 0x1bu
    };
    static const uint8_t exit_keys[] = { 0x1bu };
    char *argv[] = { "edit", "/notes.txt" };
    char long_path[600];

    file_exists = 0;
    set_input(edit_keys, sizeof(edit_keys));
    assert(editCmd(2, argv) == 0);
    assert(saved_size == 4u);
    assert(memcmp(saved_content, "AC\nB", saved_size) == 0);

    file_exists = 1;
    initial_size = MAX_FILE_SIZE;
    memset(initial_content, 'x', initial_size);
    set_input(exit_keys, sizeof(exit_keys));
    assert(editCmd(2, argv) == 0);

    initial_size = 4u;
    read_limit = 2u;
    set_input(exit_keys, sizeof(exit_keys));
    assert(editCmd(2, argv) == -1);

    memset(long_path, 'p', sizeof(long_path));
    long_path[sizeof(long_path) - 1u] = '\0';
    {
        EditorState path_editor;

        assert(editor_construct_path(&path_editor, long_path) == -1);
        memset(path_editor.buffer, '\n', MAX_LINES);
        path_editor.text_length = MAX_LINES;
        assert(editor_build_line_offsets(&path_editor) == -1);
    }
    return 0;
}
