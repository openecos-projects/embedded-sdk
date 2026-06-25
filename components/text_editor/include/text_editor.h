#ifndef __TEXT_EDITOR_H__
#define __TEXT_EDITOR_H__

#include <stdint.h>
#include <stddef.h>

/* ===== Editor Buffer Limits ===== */
#define MAX_FILE_SIZE   (24 * 1024)  /* reduced from 32KB to leave stack headroom;
                                       128KB SRAM with ~60KB BSS only leaves ~6KB
                                       stack — not enough for FIL.buf[4096] */
#define MAX_LINES       8192
#define MAX_FILENAME    256
#define LINE_NUM_WIDTH  5

/* ===== Terminal Defaults ===== */
#define DEFAULT_ROWS    24
#define DEFAULT_COLS    80

/* ===== ANSI Escape Codes ===== */
#define ANSI_CLEAR       "\033[2J\033[H"
#define ANSI_CURSOR_HIDE "\033[?25l"
#define ANSI_CURSOR_SHOW "\033[?25h"
#define ANSI_RESET       "\033[0m"
#define ANSI_BOLD        "\033[1m"
#define ANSI_CLEAR_LINE  "\033[K"

/* Foreground colors */
#define ANSI_FG_BLACK    "\033[30m"
#define ANSI_FG_RED      "\033[31m"
#define ANSI_FG_GREEN    "\033[32m"
#define ANSI_FG_YELLOW   "\033[33m"
#define ANSI_FG_WHITE    "\033[37m"
#define ANSI_FG_GRAY     "\033[90m"

/* Background colors */
#define ANSI_BG_CYAN     "\033[46m"
#define ANSI_BG_YELLOW   "\033[43m"

/* Compound styles */
#define TOP_BAR_STYLE    ANSI_BG_CYAN ANSI_FG_BLACK ANSI_BOLD
#define LINE_NUM_STYLE   ANSI_FG_GRAY
#define TEXT_STYLE       ANSI_FG_WHITE
#define BOT_BAR_STYLE    ANSI_BG_CYAN ANSI_FG_BLACK ANSI_BOLD
#define WARN_BAR_STYLE   ANSI_BG_YELLOW ANSI_FG_BLACK ANSI_BOLD
#define CURSOR_STYLE     "\033[47;30m"   /* white bg, black fg */

/* ===== Virtual Key Codes (above ASCII range) ===== */
#define KEY_ESC      0x100
#define KEY_UP       0x101
#define KEY_DOWN     0x102
#define KEY_RIGHT    0x103
#define KEY_LEFT     0x104
#define KEY_UNKNOWN  0x1FF

/* ===== Editor State ===== */
typedef struct {
    char     buffer[MAX_FILE_SIZE];       /* text buffer, \n-separated lines */
    uint16_t line_offsets[MAX_LINES];     /* byte offset of each line start */
    uint16_t line_count;                  /* number of lines */
    uint16_t text_length;                 /* total bytes in buffer */
    uint16_t cursor_row;                  /* 0-based cursor row */
    uint16_t cursor_col;                  /* 0-based cursor column */
    uint16_t scroll_row;                  /* first visible row (0-based) */
    char     filename[MAX_FILENAME];      /* display filename (no path) */
    char     fullpath[MAX_FILENAME * 2];  /* full FatFs path including drive */
    int      dirty;                       /* unsaved changes flag */
    int      rows;                        /* terminal rows */
    int      cols;                        /* terminal columns */
    int      esc_warning;                 /* ESC pressed once with unsaved changes */
    int      save_error;                  /* last save failed (transient) */
    int      buffer_full;                 /* buffer full — insertion rejected (transient) */
} EditorState;

/* ===== Public API ===== */
int editCmd(int argc, char *argv[]);

/* ===== Internal API (exposed for modular compilation) ===== */
void editor_render(EditorState *ed);
void editor_build_line_offsets(EditorState *ed);
int  editor_line_length(EditorState *ed, uint16_t line_idx);
void editor_construct_path(EditorState *ed, const char *user_path);

#endif /* __TEXT_EDITOR_H__ */
