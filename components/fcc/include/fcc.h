#ifndef __FCC_H__
#define __FCC_H__

#include <stdint.h>
#include <stddef.h>

/* ===== Code Buffer ===== */
/* Internal SRAM — must be executable; PSRAM (0x40000000+) is data-only.
 * Offset by 64 KB to keep clear of any boot/reserved area. */
#define FCC_CODE_BASE    0x30010000
#define FCC_CODE_MAX     (32 * 1024)
#define FCC_GLOBAL_BASE  (FCC_CODE_BASE + FCC_CODE_MAX)

/* ===== Source Buffer ===== */
#define FCC_SRC_MAX     (32 * 1024)

/* ===== RISC-V Register Numbers ===== */
enum {
    REG_ZERO = 0,
    REG_RA   = 1,
    REG_SP   = 2,
    REG_S0   = 8,
    REG_T0   = 5,
    REG_T1   = 6,
    REG_T2   = 7,
    REG_A0   = 10,
    REG_A1   = 11,
    REG_A2   = 12,
    REG_A3   = 13,
    REG_A4   = 14,
    REG_A5   = 15,
    REG_A6   = 16,
    REG_A7   = 17,
};

/* ===== Token Types ===== */
typedef enum {
    TOK_EOF = 0,
    /* Keywords */
    TOK_INT, TOK_RETURN, TOK_IF, TOK_ELSE, TOK_WHILE,
    /* Literals & identifiers */
    TOK_IDENT, TOK_NUMBER,
    /* Single-char */
    TOK_LBRACE,    /* { */
    TOK_RBRACE,    /* } */
    TOK_LPAREN,    /* ( */
    TOK_RPAREN,    /* ) */
    TOK_SEMI,      /* ; */
    TOK_ASSIGN,    /* = */
    TOK_COMMA,     /* , */
    /* Operators */
    TOK_PLUS,      /* + */
    TOK_MINUS,     /* - */
    TOK_STAR,      /* * */
    TOK_SLASH,     /* / */
    TOK_PERCENT,   /* % */
    TOK_EQ,        /* == */
    TOK_NE,        /* != */
    TOK_LT,        /* < */
    TOK_GT,        /* > */
    TOK_LE,        /* <= */
    TOK_GE,        /* >= */
    TOK_AND,       /* && */
    TOK_OR,        /* || */
    TOK_NOT,       /* ! */
} TokenType;

/* ===== Token ===== */
typedef struct {
    TokenType type;
    int       value;       /* TOK_NUMBER */
    char      name[64];    /* TOK_IDENT */
    int       line;        /* source line */
} Token;

/* ===== Lexer ===== */
typedef struct {
    const char *src;
    int         pos;
    int         line;
    Token       current;
} Lexer;

/* ===== Symbol Table ===== */
#define MAX_VARS  256

typedef struct {
    char name[64];
    int  offset;      /* stack offset (locals) or abs address (globals) */
    int  is_global;
} VarInfo;

/* ===== Function Table ===== */
#define MAX_FUNCS      64
#define MAX_FWD_FIXUPS 16

typedef struct {
    char name[64];
    int  label_id;                  /* label marking function entry; -1 if not yet defined */
    int  params;                    /* number of int parameters */
    int  fwd_fixups[MAX_FWD_FIXUPS]; /* fixup IDs for forward calls */
    int  fwd_count;
} FuncInfo;

/* ===== Compiler ===== */
typedef struct {
    Lexer     lexer;
    uint32_t *code;
    uint32_t  code_size;   /* bytes */
    int       error;
    /* Symbol table */
    VarInfo   vars[MAX_VARS];
    int       var_count;
    int       local_base;   /* index of first local in current function */
    int       local_bytes;  /* total bytes for current function's locals */
    int       next_global;  /* next global address offset */
    /* Function table */
    FuncInfo  funcs[MAX_FUNCS];
    int       func_count;
    char      current_func_name[64]; /* name of function being compiled */
    int       current_func_param_count;
    /* Built-in symbols */
    uint32_t  exec_func_addr;
} Compiler;

/* ===== Lexer API (used across fcc modules) ===== */
void lexer_init(Lexer *lex, const char *src);
void lexer_next(Lexer *lex);

/* ===== Public API ===== */
int fccCmd(int argc, char *argv[]);

#endif /* __FCC_H__ */
