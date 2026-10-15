#include "text_editor.h"

#include "ecos/bsp/console.h"

#include "stdio.h"
#include "string.h"

static void editor_write(const char *text)
{
    (void)bsp_console_write(text, strlen(text));
}

static void editor_putc(char ch)
{
    char visible = ((unsigned char)ch < 0x20u || ch == 0x7f) ? '?' : ch;

    (void)bsp_console_write(&visible, 1u);
}

static void editor_write_visible(const char *text, size_t length)
{
    for (size_t index = 0u; index < length; ++index)
        editor_putc(text[index]);
}

static void editor_gotoxy(int row, int col)
{
    char sequence[24];
    int length = snprintf(sequence, sizeof(sequence), "\033[%d;%dH", row, col);

    if (length > 0 && (size_t)length < sizeof(sequence))
        (void)bsp_console_write(sequence, (size_t)length);
}

static void editor_fill_line(EditorState *ed, int used)
{
    for (int remaining = ed->cols - used; remaining > 0; --remaining)
        editor_putc(' ');
}

static void editor_render_top_bar(EditorState *ed)
{
    char title[MAX_FILENAME + 8];
    size_t used;

    editor_gotoxy(1, 1);
    editor_write(TOP_BAR_STYLE);
    (void)snprintf(title, sizeof(title), ed->dirty ? "  [%s]*" : "  [%s]",
                   ed->filename);
    used = strlen(title);
    if (used > (size_t)ed->cols) used = (size_t)ed->cols;
    editor_write_visible(title, used);
    editor_fill_line(ed, (int)used);
    editor_write(ANSI_RESET);
}

static void editor_render_content_line(EditorState *ed, int screen_row,
                                       uint16_t line_idx)
{
    char number[16];
    int offset = ed->line_offsets[line_idx];
    int len = editor_line_length(ed, line_idx);
    int printed = LINE_NUM_WIDTH;
    int is_cursor_line = line_idx == ed->cursor_row;

    editor_gotoxy(screen_row, 1);
    editor_write(LINE_NUM_STYLE);
    (void)snprintf(number, sizeof(number), "%4u ", (unsigned)line_idx + 1u);
    editor_write(number);
    editor_write(ANSI_RESET TEXT_STYLE);

    for (int index = 0; index < len && printed < ed->cols; ++index) {
        char ch = ed->buffer[offset + index];

        if (is_cursor_line && index == (int)ed->cursor_col)
            editor_write(ANSI_RESET CURSOR_STYLE);

        if (ch == '\t') {
            int spaces = 4 - (printed % 4);

            while (spaces-- > 0 && printed < ed->cols) {
                editor_putc(' ');
                ++printed;
            }
        } else {
            editor_putc(ch);
            ++printed;
        }

        if (is_cursor_line && index == (int)ed->cursor_col)
            editor_write(ANSI_RESET TEXT_STYLE);
    }

    if (is_cursor_line && ed->cursor_col >= (uint16_t)len && printed < ed->cols) {
        editor_write(ANSI_RESET CURSOR_STYLE);
        editor_putc(' ');
    }
    editor_write(ANSI_RESET ANSI_CLEAR_LINE);
}

static void editor_render_bottom_bar(EditorState *ed)
{
    char status[128];
    size_t used;

    editor_gotoxy(ed->rows, 1);
    if (ed->esc_warning) {
        editor_write(WARN_BAR_STYLE);
        (void)snprintf(status, sizeof(status),
                       "  Not saved! Press Esc again to confirm exit");
    } else if (ed->save_error) {
        editor_write(WARN_BAR_STYLE);
        (void)snprintf(status, sizeof(status), "  Save failed!");
    } else if (ed->buffer_full) {
        editor_write(WARN_BAR_STYLE);
        (void)snprintf(status, sizeof(status), "  Buffer full!");
    } else {
        editor_write(BOT_BAR_STYLE);
        (void)snprintf(status, sizeof(status),
                       "  Ln %u, Col %u | Ctrl+W Save  Esc Exit",
                       (unsigned)ed->cursor_row + 1u,
                       (unsigned)ed->cursor_col + 1u);
    }

    used = strlen(status);
    if (used > (size_t)ed->cols) used = (size_t)ed->cols;
    editor_write_visible(status, used);
    editor_fill_line(ed, (int)used);
    editor_write(ANSI_RESET);
}

void editor_render(EditorState *ed)
{
    int content_height = ed->rows - 2;

    editor_write("\033[H" ANSI_CURSOR_HIDE);
    editor_render_top_bar(ed);
    if (content_height < 1) content_height = 1;

    for (int index = 0; index < content_height; ++index) {
        uint16_t line_idx = ed->scroll_row + (uint16_t)index;

        if (line_idx < ed->line_count) {
            editor_render_content_line(ed, index + 2, line_idx);
        } else {
            editor_gotoxy(index + 2, 1);
            editor_write(ANSI_RESET "~" ANSI_CLEAR_LINE);
        }
    }

    editor_render_bottom_bar(ed);
    editor_write(ANSI_CURSOR_SHOW);
}
