#include "text_editor.h"
#include "stdio.h"
#include "string.h"
#include "generated/autoconf.h"

#ifdef CONFIG_COMPONENT_FLASH_FS
#include "ff.h"
#endif

#include "shell.h"
#include "shell_port.h"
#include "hal_sys_uart.h"
#include "board.h"

/* Poll timeout: max iterations waiting for '[' after ESC.  Each iteration
 * does one MMIO read; 2000 iterations ≈ tens of ms — fast enough for
 * standalone ESC, but long enough to catch arrow-key CSI sequences. */
#define ESC_POLL_MAX  2000

/* ===== Singleton Editor State ===== */
static EditorState editor;

/* Properly-typed reference to ff.c's global FatFs array.
 * NOTE: Must stay a declaration — adding static definitions here
 * bloats BSS and triggers stack/BSS collisions during boot. */
#ifdef CONFIG_COMPONENT_FLASH_FS
extern FATFS* FatFs[FF_VOLUMES];
#endif


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
void editor_build_line_offsets(EditorState *ed) {
    ed->line_count   = 0;
    ed->line_offsets[0] = 0;
    ed->line_count   = 1;

    for (uint16_t i = 0; i < ed->text_length; i++) {
        if (ed->buffer[i] == '\n') {
            if (ed->line_count >= MAX_LINES) break;
            ed->line_offsets[ed->line_count] = i + 1;
            ed->line_count++;
        }
    }
}

/**
 * @brief Construct the full FatFs path from a user-supplied path.
 *
 * If the user path starts with '/', it's treated as absolute and
 * prefixed with "0:". Otherwise, it's relative to the shell's
 * current working directory.
 */
void editor_construct_path(EditorState *ed, const char *user_path) {
#ifdef CONFIG_COMPONENT_FLASH_FS
    Shell *shell = shellGetCurrent();
    const char *cwd = "/";

    if (shell != NULL) {
        cwd = shellGetPath(shell);
        if (cwd == NULL) cwd = "/";
    }

    if (user_path[0] == '/') {
        /* Absolute path: 0:<user_path> */
        snprintf(ed->fullpath, sizeof(ed->fullpath), "0:%s", user_path);
    } else {
        /* Relative path: 0:<cwd>/<user_path>  (handle trailing / in cwd) */
        size_t cwd_len = strlen(cwd);
        if (cwd_len > 0 && cwd[cwd_len - 1] == '/') {
            snprintf(ed->fullpath, sizeof(ed->fullpath),
                     "0:%s%s", cwd, user_path);
        } else {
            snprintf(ed->fullpath, sizeof(ed->fullpath),
                     "0:%s/%s", cwd, user_path);
        }
    }
#endif
}

/**
 * @brief Load a file from FatFs into the editor buffer.
 *
 * Returns 0 on success, -1 on a hard error (file too large, I/O error
 * that makes editing impossible).  Returns 0 for "file not found" since
 * that means "create new file" — an empty buffer is valid.
 */
static int editor_load_file(EditorState *ed) {
#ifdef CONFIG_COMPONENT_FLASH_FS
    FIL file;
    FRESULT res;

    res = f_open(&file, ed->fullpath, FA_OPEN_EXISTING | FA_READ);
    if (res == FR_NO_FILE) {
        /* New file: empty buffer is fine */
        ed->text_length = 0;
        ed->buffer[0]   = '\0';
        editor_build_line_offsets(ed);
        return 0;
    }

    if (res != FR_OK) {
        printf("\r\nError: cannot open file (code %d)\r\n", res);
        return -1;
    }

    /* Check file size */
    if (f_size(&file) > MAX_FILE_SIZE) {
        printf("\r\nError: file too large (max %d bytes)\r\n", MAX_FILE_SIZE);
        f_close(&file);
        return -1;
    }

    /* Read entire file */
    UINT bytes_read;
    res = f_read(&file, ed->buffer, MAX_FILE_SIZE, &bytes_read);
    f_close(&file);

    if (res != FR_OK) {
        printf("\r\nError: read failed (code %d)\r\n", res);
        return -1;
    }

    ed->text_length = (uint16_t)bytes_read;
    ed->buffer[ed->text_length] = '\0';
    editor_build_line_offsets(ed);
    return 0;
#else
    return -1;
#endif
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
    editor_construct_path(ed, filename);

    /* Load file from Flash */
    return editor_load_file(ed);
}

/* ===== Input Handling ===== */

/**
 * @brief Non-blocking poll of the UART data register.
 *        Returns the received byte (0..255), or -1 if no data.
 */
static int uart_poll(void) {
    int32_t c = REG_UART_0_DATA;
    return (c == -1) ? -1 : (int)c;
}

/**
 * @brief Read a single key from the terminal.
 *
 * Handles ANSI escape sequences (arrow keys).  After receiving ESC (0x1B),
 * polls for a short window to see if '[' follows.  If so, reads the final
 * direction byte and returns a virtual key code.  If the window expires,
 * treats the ESC as a standalone key press.
 */
static int editor_read_key(void) {
    int c = (int)hal_sys_getchar();

    if (c != 0x1B) {
        return c;  /* Plain ASCII character */
    }

    /*
     * ESC received.  Poll the UART register for '[' within a short
     * window.  For arrow keys the '[' byte is already in-flight and
     * appears almost instantly; for a standalone ESC the poll expires
     * and we return KEY_ESC.  Filter out re-reads of the ESC byte
     * itself — some UARTs need a cycle to clear the "data ready" flag.
     */
    for (volatile int i = 0; i < ESC_POLL_MAX; i++) {
        int next = uart_poll();
        if (next >= 0 && next != 0x1B) {
            if (next == '[') {
                /* ANSI / VT100: ESC [ A/B/C/D */
                int dir = (int)hal_sys_getchar();
                switch (dir) {
                    case 'A': return KEY_UP;
                    case 'B': return KEY_DOWN;
                    case 'C': return KEY_RIGHT;
                    case 'D': return KEY_LEFT;
                    default:  return KEY_UNKNOWN;
                }
            }
            /* VT52: ESC A/B/C/D (no '[' byte) */
            switch (next) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                default:  return KEY_UNKNOWN;
            }
        }
    }

    /* No CSI sequence — standalone ESC */
    return KEY_ESC;
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
        }

        ed->cursor_row--;
        ed->cursor_col = prev_len;
        editor_build_line_offsets(ed);
        editor_scroll_to_cursor(ed);
        ed->dirty = 1;
    }
}

/**
 * @brief Insert a newline at the cursor position, splitting the line.
 */
static void editor_insert_newline(EditorState *ed) {
    if (ed->text_length >= MAX_FILE_SIZE) return;
    if (ed->line_count >= MAX_LINES) return;

    uint16_t pos = ed->line_offsets[ed->cursor_row] + ed->cursor_col;

    /* Shift text right, insert \n */
    memmove(&ed->buffer[pos + 1], &ed->buffer[pos],
            ed->text_length - pos);
    ed->buffer[pos] = '\n';
    ed->text_length++;

    /* Cursor moves to start of the new line */
    ed->cursor_row++;
    ed->cursor_col = 0;

    editor_build_line_offsets(ed);
    editor_scroll_to_cursor(ed);
    ed->dirty = 1;
}

/* ===== File I/O ===== */

/**
 * @brief Save the editor buffer to Flash via FatFs.
 *        On success clears the dirty flag.  On failure sets save_error.
 */
static void editor_save(EditorState *ed) {
#ifdef CONFIG_COMPONENT_FLASH_FS
    FIL file;
    FRESULT res;

    res = f_open(&file, ed->fullpath, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        ed->save_error = 1;
        return;
    }

    UINT bytes_written;
    res = f_write(&file, ed->buffer, ed->text_length, &bytes_written);
    f_close(&file);

    if (res == FR_OK && bytes_written == ed->text_length) {
        ed->dirty = 0;
        ed->save_error = 0;
    } else {
        ed->save_error = 1;
    }
#endif
}

/* ===== Editor Loop ===== */

static void editor_loop(EditorState *ed) {
    int running = 1;

    /* Clear screen once on entry — subsequent frames overwrite in place */
    printf("\033[2J");

    ed->esc_warning = 0;
    ed->save_error  = 0;

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

            case '\r':                              /* Enter (CR) */
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
    printf(ANSI_CLEAR ANSI_CURSOR_SHOW);
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
        printf("\r\nUsage: edit <filename>\r\n");
        return -1;
    }

#ifdef CONFIG_COMPONENT_FLASH_FS
    if (editor_init(&editor, argv[1]) != 0) {
        return -1;  /* editor_load_file already printed the error */
    }

    editor_loop(&editor);
#endif

    return 0;
}
