#include "text_editor.h"
#include "stdio.h"
#include "hal_sys_uart.h"

/**
 * @brief Output a single character to UART
 */
static inline void editor_putc(char c) {
    hal_sys_putchar(c);
}

/**
 * @brief ANSI cursor position: \033[row;colH (1-based)
 */
static void editor_gotoxy(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

/**
 * @brief Fill remaining width with spaces in current style
 */
static void editor_fill_line(EditorState *ed, int used) {
    int remaining = ed->cols - used;
    while (remaining-- > 0) editor_putc(' ');
}

/**
 * @brief Render top bar: [filename] with optional dirty marker
 */
static void editor_render_top_bar(EditorState *ed) {
    editor_gotoxy(1, 1);
    printf(TOP_BAR_STYLE);

    char title[128];
    int dirty_len = 0;
    if (ed->dirty) {
        dirty_len = snprintf(title, sizeof(title), "  [%s]*", ed->filename);
    } else {
        dirty_len = snprintf(title, sizeof(title), "  [%s]", ed->filename);
    }
    printf("%s", title);
    editor_fill_line(ed, dirty_len);
    printf(ANSI_RESET);
}

/**
 * @brief Render one content line: line number + text.
 *        If this line contains the cursor, the character at the cursor
 *        position is rendered with inverse-video highlighting.
 */
static void editor_render_content_line(EditorState *ed, int screen_row,
                                        uint16_t line_idx) {
    editor_gotoxy(screen_row, 1);

    /* Line number */
    printf(LINE_NUM_STYLE);
    printf("%4d ", line_idx + 1);
    printf(ANSI_RESET);

    int offset         = ed->line_offsets[line_idx];
    int len            = editor_line_length(ed, line_idx);
    int printed        = LINE_NUM_WIDTH;
    int is_cursor_line = (line_idx == ed->cursor_row);

    printf(TEXT_STYLE);
    for (int i = 0; i < len; i++) {
        /* Switch to cursor highlight at the cursor column */
        if (is_cursor_line && i == (int)ed->cursor_col) {
            printf(ANSI_RESET CURSOR_STYLE);
        }

        char ch = ed->buffer[offset + i];
        if (ch == '\t') {
            /* Expand tabs to spaces (4-char tab stop) */
            int spaces = 4 - (printed % 4);
            if (spaces == 0) spaces = 4;
            while (spaces-- > 0 && printed < ed->cols) {
                editor_putc(' ');
                printed++;
            }
        } else {
            editor_putc(ch);
            printed++;
        }

        /* Switch back to normal text after the cursor character */
        if (is_cursor_line && i == (int)ed->cursor_col) {
            printf(ANSI_RESET TEXT_STYLE);
        }

        if (printed >= ed->cols) break;
    }

    /* Cursor at or past end of line: show a highlighted block (space) */
    if (is_cursor_line && ed->cursor_col >= (uint16_t)len) {
        printf(ANSI_RESET CURSOR_STYLE);
        editor_putc(' ');
        printed++;
    }

    printf(ANSI_RESET ANSI_CLEAR_LINE);
}

/**
 * @brief Render bottom bar: status line.
 *
 * Three modes:
 *   Normal   — cyan:   Ln / Col / key hints
 *   Warning  — yellow: unsaved-changes exit confirmation prompt
 *   Error    — yellow: last save failed
 */
static void editor_render_bottom_bar(EditorState *ed) {
    editor_gotoxy(ed->rows, 1);

    int printed;
    if (ed->esc_warning) {
        printf(WARN_BAR_STYLE);
        printed = printf("  Not saved! Press Esc again to confirm exit");
    } else if (ed->save_error) {
        printf(WARN_BAR_STYLE);
        printed = printf("  Save failed!");
    } else if (ed->buffer_full) {
        printf(WARN_BAR_STYLE);
        printed = printf("  Buffer full!");
    } else {
        printf(BOT_BAR_STYLE);
        printed = printf("  Ln %u, Col %u | Ctrl+W Save  Esc Exit",
                         ed->cursor_row + 1, ed->cursor_col + 1);
    }

    int remaining = ed->cols - printed;
    while (remaining-- > 0) editor_putc(' ');
    printf(ANSI_RESET);
}

/**
 * @brief Full editor render: top bar, content area, bottom bar
 *
 * Redraws the entire screen on every call. Content area occupies
 * rows 2..(ed->rows - 1), displaying up to (ed->rows - 2) lines
 * starting from ed->scroll_row.
 */
void editor_render(EditorState *ed) {
    /* Home cursor + hide — no clear-screen to avoid flicker.
     * Every visible line is overwritten positionally below. */
    printf("\033[H" ANSI_CURSOR_HIDE);

    editor_render_top_bar(ed);

    int content_height = ed->rows - 2;
    if (content_height < 1) content_height = 1;

    for (int i = 0; i < content_height; i++) {
        uint16_t line_idx = ed->scroll_row + (uint16_t)i;
        if (line_idx < ed->line_count) {
            editor_render_content_line(ed, 2 + i, line_idx);
        } else {
            /* Empty line after end of file */
            editor_gotoxy(2 + i, 1);
            printf(ANSI_RESET "~" ANSI_CLEAR_LINE);
        }
    }

    editor_render_bottom_bar(ed);
    printf(ANSI_CURSOR_SHOW);
}
