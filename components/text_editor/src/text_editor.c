#include "text_editor.h"
#include "stdio.h"
#include "string.h"
#include "ff.h"
#include "ecos/bsp/console.h"
#include "ecos/log.h"

#include "shell.h"

/* Bound escape-sequence polling so a standalone ESC remains usable. */
#define ESC_POLL_MAX  20000u
#define KEY_IO_ERROR  0x1FE

/* ===== Singleton Editor State ===== */
static EditorState editor;
static int pending_byte = -1;


/* ===== Helpers ===== */

/**
 * @brief Get the length of a line (excluding \n).
 *        For the last line, measures to text_length.
 */
int editor_line_length(EditorState *ed, uint16_t line_idx) {
    if (line_idx >= ed->line_count) return 0;

    uint16_t start = ed->line_offsets[line_idx];
    uint16_t end;

    if (line_idx + 1 < ed->line_count) {
        end = ed->line_offsets[line_idx + 1];
        /* If the line ends with \n, exclude it */
        if (end > start && ed->buffer[end - 1] == '\n') {
            end--;
        }
    } else {
        end = ed->text_length;
    }

    return (int)(end - start);
}

/**
 * @brief Scan the text buffer and build the line_offsets array.
 */
int editor_build_line_offsets(EditorState *ed) {
    ed->line_count   = 0;
    ed->line_offsets[0] = 0;
    ed->line_count   = 1;

    for (uint16_t i = 0; i < ed->text_length; i++) {
        if (ed->buffer[i] == '\n') {
            if (ed->line_count >= MAX_LINES) return -1;
            ed->line_offsets[ed->line_count] = i + 1;
            ed->line_count++;
        }
    }
    return 0;
}

/**
 * @brief Construct the full FatFs path from a user-supplied path.
 *
 * If the user path starts with '/', it's treated as absolute and
 * prefixed with "0:". Otherwise, it's relative to the shell's
 * current working directory.
 */
int editor_construct_path(EditorState *ed, const char *user_path) {
    Shell *shell = shellGetCurrent();
    const char *cwd = "/";
    int length;

    if (shell != NULL) {
        cwd = shellGetPath(shell);
        if (cwd == NULL) cwd = "/";
    }

    if (user_path[0] == '/') {
        /* Absolute path: 0:<user_path> */
        length = snprintf(ed->fullpath, sizeof(ed->fullpath), "0:%s", user_path);
    } else {
        /* Relative path: 0:<cwd>/<user_path>  (handle trailing / in cwd) */
        size_t cwd_len = strlen(cwd);
        if (cwd_len > 0 && cwd[cwd_len - 1] == '/') {
            length = snprintf(ed->fullpath, sizeof(ed->fullpath),
                              "0:%s%s", cwd, user_path);
        } else {
            length = snprintf(ed->fullpath, sizeof(ed->fullpath),
                              "0:%s/%s", cwd, user_path);
        }
    }
    return length >= 0 && (size_t)length < sizeof(ed->fullpath) ? 0 : -1;
}

/**
 * @brief Load a file from FatFs into the editor buffer.
 *
 * Returns 0 on success, -1 on a hard error (file too large, I/O error
 * that makes editing impossible).  Returns 0 for "file not found" since
 * that means "create new file" — an empty buffer is valid.
 */
static int editor_load_file(EditorState *ed) {
    FIL file;
    FRESULT res;
    FRESULT close_result;
    FSIZE_t file_size;

    res = f_open(&file, ed->fullpath, FA_OPEN_EXISTING | FA_READ);
    if (res == FR_NO_FILE) {
        /* New file: empty buffer is fine */
        ed->text_length = 0;
        ed->buffer[0]   = '\0';
        (void)editor_build_line_offsets(ed);
        return 0;
    }

    if (res != FR_OK) {
        (void)ECOS_LOGE("text-editor", "cannot open file (code %d)", res);
        return -1;
    }

    /* Check file size */
    file_size = f_size(&file);
    if (file_size > MAX_FILE_SIZE) {
        (void)ECOS_LOGE("text-editor", "file too large (max %d bytes)", MAX_FILE_SIZE);
        f_close(&file);
        return -1;
    }

    /* Read entire file */
    UINT bytes_read;
    res = f_read(&file, ed->buffer, MAX_FILE_SIZE, &bytes_read);
    close_result = f_close(&file);

    if (res != FR_OK || close_result != FR_OK || bytes_read != file_size) {
        (void)ECOS_LOGE("text-editor", "read failed (code %d)",
                        res != FR_OK ? res : close_result);
        return -1;
    }

    ed->text_length = (uint16_t)bytes_read;
    ed->buffer[ed->text_length] = '\0';
    if (editor_build_line_offsets(ed) != 0) {
        (void)ECOS_LOGE("text-editor", "file has too many lines (max %d)", MAX_LINES);
        return -1;
    }
    return 0;
}

/* ===== Editor Init ===== */

/**
 * @brief Initialize editor state and load file.  Returns 0 on success,
 *        -1 if the file cannot be loaded (too large, I/O error).
 */
static int editor_init(EditorState *ed, const char *filename) {
    memset(ed, 0, sizeof(EditorState));
    ed->rows        = DEFAULT_ROWS;
    ed->cols        = DEFAULT_COLS;
    ed->cursor_row  = 0;
    ed->cursor_col  = 0;
    ed->scroll_row  = 0;
    ed->dirty       = 0;

    /* Extract display filename (basename) */
    const char *basename = filename;
    for (const char *p = filename; *p != '\0'; p++) {
        if (*p == '/') basename = p + 1;
    }
    snprintf(ed->filename, sizeof(ed->filename), "%s", basename);

    /* Construct full FatFs path */
    if (editor_construct_path(ed, filename) != 0) {
        (void)ECOS_LOGE("text-editor", "file path is too long");
        return -1;
    }

    /* Load file from Flash */
    return editor_load_file(ed);
}

/* ===== Input Handling ===== */

static int editor_poll_byte(void) {
    uint8_t byte;

    for (unsigned int attempt = 0; attempt < ESC_POLL_MAX; ++attempt) {
        int result = bsp_console_try_read(&byte);

        if (result == 1) return byte;
        if (result < 0) return -2;
    }
    return -1;
}

/**
 * @brief Read a single key from the terminal.
 *
 * Handles ANSI/VT52 arrow keys without waiting forever after a lone ESC.
 */
static int editor_read_key(void) {
    uint8_t byte;
    int c;
    int csi = 0;

    if (pending_byte >= 0) {
        c = pending_byte;
        pending_byte = -1;
    } else {
        int result = bsp_console_read(&byte, 1u);
        if (result != 1) return KEY_IO_ERROR;
        c = byte;
    }

    if (c != 0x1B) {
        return c;
    }

    int next = editor_poll_byte();
    if (next == -2) return KEY_IO_ERROR;
    if (next < 0) return KEY_ESC;

    if (next == '[') {
        csi = 1;
        next = editor_poll_byte();
        if (next == -2) return KEY_IO_ERROR;
        if (next < 0) return KEY_UNKNOWN;
    }

    switch (next) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:
            if (csi) return KEY_UNKNOWN;
            pending_byte = next;
            return KEY_ESC;
    }
}

/* ===== Cursor Movement ===== */

/**
 * @brief Adjust scroll_row so that cursor_row is visible.
 */
static void editor_scroll_to_cursor(EditorState *ed) {
    int content_height = ed->rows - 2;
    if (content_height < 1) content_height = 1;

    if (ed->cursor_row < ed->scroll_row) {
        ed->scroll_row = ed->cursor_row;
    } else if (ed->cursor_row >= ed->scroll_row + (uint16_t)content_height) {
        ed->scroll_row = ed->cursor_row - (uint16_t)content_height + 1;
    }
}

/**
 * @brief Clamp cursor_col so it does not exceed the current line's length.
 */
static void editor_clamp_cursor_col(EditorState *ed) {
    int line_len = editor_line_length(ed, ed->cursor_row);
    if (ed->cursor_col > (uint16_t)line_len) {
        ed->cursor_col = (uint16_t)line_len;
    }
}

static void editor_move_up(EditorState *ed) {
    if (ed->cursor_row > 0) {
        ed->cursor_row--;
        editor_clamp_cursor_col(ed);
        editor_scroll_to_cursor(ed);
    }
}

static void editor_move_down(EditorState *ed) {
    if (ed->cursor_row + 1 < ed->line_count) {
        ed->cursor_row++;
        editor_clamp_cursor_col(ed);
        editor_scroll_to_cursor(ed);
    }
}

static void editor_move_left(EditorState *ed) {
    if (ed->cursor_col > 0) {
        ed->cursor_col--;
    } else if (ed->cursor_row > 0) {
        /* Move to end of previous line */
        ed->cursor_row--;
        ed->cursor_col = (uint16_t)editor_line_length(ed, ed->cursor_row);
        editor_scroll_to_cursor(ed);
    }
}

static void editor_move_right(EditorState *ed) {
    int line_len = editor_line_length(ed, ed->cursor_row);

    if (ed->cursor_col < (uint16_t)line_len) {
        ed->cursor_col++;
    } else if (ed->cursor_row + 1 < ed->line_count) {
        /* Move to start of next line */
        ed->cursor_row++;
        ed->cursor_col = 0;
        editor_scroll_to_cursor(ed);
    }
}

/* ===== Text Editing ===== */

/**
 * @brief Insert a single character at the cursor position.
 *        Shifts subsequent text right and updates line offsets.
 */
static void editor_insert_char(EditorState *ed, char ch) {
    if (ed->text_length >= MAX_FILE_SIZE) {
        ed->buffer_full = 1;
        return;
    }

    uint16_t pos = ed->line_offsets[ed->cursor_row] + ed->cursor_col;

    /* Shift text after cursor right by one byte */
    memmove(&ed->buffer[pos + 1], &ed->buffer[pos],
            ed->text_length - pos);
    ed->buffer[pos] = ch;
    ed->text_length++;
    ed->buffer[ed->text_length] = '\0';

    /* All subsequent line offsets shift by 1 */
    for (uint16_t i = ed->cursor_row + 1; i < ed->line_count; i++) {
        ed->line_offsets[i]++;
    }

    ed->cursor_col++;
    ed->dirty = 1;
}

/**
 * @brief Delete the character before the cursor (Backspace).
 *
 * If cursor is within a line, removes the preceding character.
 * If cursor is at the start of a non-first line, merges that line
 * with the previous one by removing the intervening \n.
 */
static void editor_delete_char(EditorState *ed) {
    if (ed->cursor_col > 0) {
        /* Delete char before cursor on the same line */
        uint16_t pos = ed->line_offsets[ed->cursor_row] + ed->cursor_col - 1;

        memmove(&ed->buffer[pos], &ed->buffer[pos + 1],
                ed->text_length - pos - 1);
        ed->text_length--;
        ed->buffer[ed->text_length] = '\0';
        ed->cursor_col--;

        for (uint16_t i = ed->cursor_row + 1; i < ed->line_count; i++) {
            ed->line_offsets[i]--;
        }

        ed->dirty = 1;
    } else if (ed->cursor_row > 0) {
        /* At line start — merge with previous line */
        uint16_t prev_len = (uint16_t)editor_line_length(ed,
                                                          ed->cursor_row - 1);
        uint16_t merge_pos = ed->line_offsets[ed->cursor_row] - 1;

        /* Remove the \n between the two lines (if present) */
        if (merge_pos < ed->text_length
            && ed->buffer[merge_pos] == '\n') {
            memmove(&ed->buffer[merge_pos], &ed->buffer[merge_pos + 1],
                    ed->text_length - merge_pos - 1);
            ed->text_length--;
            ed->buffer[ed->text_length] = '\0';
        }

        ed->cursor_row--;
        ed->cursor_col = prev_len;
        (void)editor_build_line_offsets(ed);
        editor_scroll_to_cursor(ed);
        ed->dirty = 1;
    }
}

/**
 * @brief Insert a newline at the cursor position, splitting the line.
 */
static void editor_insert_newline(EditorState *ed) {
    if (ed->text_length >= MAX_FILE_SIZE || ed->line_count >= MAX_LINES) {
        ed->buffer_full = 1;
        return;
    }

    uint16_t pos = ed->line_offsets[ed->cursor_row] + ed->cursor_col;

    /* Shift text right, insert \n */
    memmove(&ed->buffer[pos + 1], &ed->buffer[pos],
            ed->text_length - pos);
    ed->buffer[pos] = '\n';
    ed->text_length++;
    ed->buffer[ed->text_length] = '\0';

    /* Cursor moves to start of the new line */
    ed->cursor_row++;
    ed->cursor_col = 0;

    (void)editor_build_line_offsets(ed);
    editor_scroll_to_cursor(ed);
    ed->dirty = 1;
}

/* ===== File I/O ===== */

/**
 * @brief Save the editor buffer to Flash via FatFs.
 *        On success clears the dirty flag.  On failure sets save_error.
 */
static void editor_save(EditorState *ed) {
    FIL file;
    FRESULT res;
    FRESULT close_result;

    res = f_open(&file, ed->fullpath, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        ed->save_error = 1;
        return;
    }

    UINT bytes_written;
    res = f_write(&file, ed->buffer, ed->text_length, &bytes_written);
    close_result = f_close(&file);

    if (res == FR_OK && close_result == FR_OK && bytes_written == ed->text_length) {
        ed->dirty = 0;
        ed->save_error = 0;
    } else {
        ed->save_error = 1;
    }
}

/* ===== Editor Loop ===== */

static int editor_loop(EditorState *ed) {
    int running = 1;
    int status = 0;

    /* Clear screen once on entry — subsequent frames overwrite in place */
    (void)bsp_console_write("\033[2J", sizeof("\033[2J") - 1u);

    ed->esc_warning = 0;
    ed->save_error  = 0;
    pending_byte = -1;

    while (running) {
        editor_render(ed);

        int key = editor_read_key();
        ed->save_error  = 0;  /* transient — clears on any key */
        ed->buffer_full = 0;  /* transient — clears on any key */

        /* ESC warning state: waiting for confirmation */
        if (ed->esc_warning) {
            if (key == KEY_ESC) {
                running = 0;          /* confirmed — discard changes */
            } else {
                ed->esc_warning = 0;  /* dismissed — back to editing */
            }
            continue;
        }

        switch (key) {
            case KEY_UP:    editor_move_up(ed);    break;
            case KEY_DOWN:  editor_move_down(ed);  break;
            case KEY_LEFT:  editor_move_left(ed);  break;
            case KEY_RIGHT: editor_move_right(ed); break;

            case KEY_IO_ERROR:
                status = -1;
                running = 0;
                break;

            case 0x17:                              /* Ctrl+W — save */
                editor_save(ed);
                break;

            case KEY_ESC:
                if (ed->dirty) {
                    ed->esc_warning = 1;
                } else {
                    running = 0;
                }
                break;

            case '\n':                              /* Console normalizes CR to LF */
                editor_insert_newline(ed);
                break;

            case 0x08:                              /* Backspace */
            case 0x7F:                              /* Delete */
                editor_delete_char(ed);
                break;

            case 0x09:                              /* Tab — insert 4 spaces */
                if (ed->text_length + 4 > MAX_FILE_SIZE) {
                    ed->buffer_full = 1;
                } else {
                    editor_insert_char(ed, ' ');
                    editor_insert_char(ed, ' ');
                    editor_insert_char(ed, ' ');
                    editor_insert_char(ed, ' ');
                }
                break;

            default:
                if (key >= 0x20 && key <= 0x7E) {
                    editor_insert_char(ed, (char)key);
                }
                break;
        }
    }

    /* Clean up: clear screen before returning to shell */
    (void)bsp_console_write(ANSI_CLEAR ANSI_CURSOR_SHOW,
                            sizeof(ANSI_CLEAR ANSI_CURSOR_SHOW) - 1u);
    return status;
}

/* ===== Shell Command Entry Point ===== */

/**
 * @brief edit command: edit <filename>
 *
 * Registered as SHELL_TYPE_CMD_MAIN in the shell command table.
 * Opens a full-screen text editor for the given file.
 */
int editCmd(int argc, char *argv[]) {
    if (argc < 2) {
        (void)ECOS_LOGI("text-editor", "Usage: edit <filename>");
        return -1;
    }

    if (editor_init(&editor, argv[1]) != 0) {
        return -1;  /* editor_load_file already printed the error */
    }

    return editor_loop(&editor);
}
