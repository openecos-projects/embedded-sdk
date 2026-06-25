#include "fcc.h"
#include "string.h"

/* ===== Character predicates ===== */
static int is_alpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(int c) {
    return c >= '0' && c <= '9';
}

static int is_alnum(int c) {
    return is_alpha(c) || is_digit(c);
}

/* ===== Keyword lookup ===== */
static TokenType lookup_keyword(const char *name) {
    if (strcmp(name, "int")    == 0) return TOK_INT;
    if (strcmp(name, "return") == 0) return TOK_RETURN;
    if (strcmp(name, "if")     == 0) return TOK_IF;
    if (strcmp(name, "else")   == 0) return TOK_ELSE;
    if (strcmp(name, "while")  == 0) return TOK_WHILE;
    return TOK_IDENT;
}

/* ===== Single-char token lookup ===== */
static TokenType lookup_single(int c) {
    switch (c) {
        case '{': return TOK_LBRACE;
        case '}': return TOK_RBRACE;
        case '(': return TOK_LPAREN;
        case ')': return TOK_RPAREN;
        case ';': return TOK_SEMI;
        case ',': return TOK_COMMA;
        case '+': return TOK_PLUS;
        case '-': return TOK_MINUS;
        case '*': return TOK_STAR;
        case '/': return TOK_SLASH;
        case '%': return TOK_PERCENT;
        default:  return TOK_EOF;
    }
}

/* ===== Peek current character ===== */
static int lexer_peek(Lexer *lex) {
    if (lex->pos >= (int)strlen(lex->src)) return -1;
    return (unsigned char)lex->src[lex->pos];
}

/* ===== Advance one character ===== */
static int lexer_advance(Lexer *lex) {
    int c = lexer_peek(lex);
    if (c >= 0) {
        lex->pos++;
        if (c == '\n') lex->line++;
    }
    return c;
}

/* ===== Skip whitespace and comments ===== */
static void lexer_skip_whitespace(Lexer *lex) {
    while (1) {
        int c = lexer_peek(lex);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            lexer_advance(lex);
        } else if (c == '/') {
            /* Check for // comment */
            if (lex->pos + 1 < (int)strlen(lex->src)
                && lex->src[lex->pos + 1] == '/') {
                while (lexer_peek(lex) >= 0 && lexer_peek(lex) != '\n')
                    lexer_advance(lex);
            } else {
                break; /* division operator, let parser handle */
            }
        } else {
            break;
        }
    }
}

/* ===== Read next token into lex->current ===== */
void lexer_next(Lexer *lex) {
    lexer_skip_whitespace(lex);

    int c = lexer_peek(lex);
    if (c < 0) {
        lex->current.type  = TOK_EOF;
        lex->current.value = 0;
        lex->current.line  = lex->line;
        return;
    }

    /* Remember line for this token */
    int tok_line = lex->line;

    /* Identifier or keyword */
    if (is_alpha(c)) {
        char name[64];
        int len = 0;
        while (is_alnum(lexer_peek(lex)) && len < 63) {
            name[len++] = (char)lexer_advance(lex);
        }
        name[len] = '\0';

        lex->current.type = lookup_keyword(name);
        lex->current.value = 0;
        lex->current.line  = tok_line;
        if (lex->current.type == TOK_IDENT) {
            strncpy(lex->current.name, name, sizeof(lex->current.name) - 1);
            lex->current.name[sizeof(lex->current.name) - 1] = '\0';
        }
        return;
    }

    /* Number */
    if (is_digit(c)) {
        int value = 0;
        while (is_digit(lexer_peek(lex))) {
            value = value * 10 + (lexer_advance(lex) - '0');
        }
        lex->current.type  = TOK_NUMBER;
        lex->current.value = value;
        lex->current.line  = tok_line;
        return;
    }

    /* Two-char operators */
    if (c == '=') {
        lexer_advance(lex);
        if (lexer_peek(lex) == '=') {
            lexer_advance(lex);
            lex->current.type = TOK_EQ;
        } else {
            lex->current.type = TOK_ASSIGN;
        }
        lex->current.value = 0;
        lex->current.line  = tok_line;
        return;
    }
    if (c == '!') {
        lexer_advance(lex);
        if (lexer_peek(lex) == '=') {
            lexer_advance(lex);
            lex->current.type = TOK_NE;
        } else {
            lex->current.type = TOK_NOT;
        }
        lex->current.value = 0;
        lex->current.line  = tok_line;
        return;
    }
    if (c == '<') {
        lexer_advance(lex);
        if (lexer_peek(lex) == '=') {
            lexer_advance(lex);
            lex->current.type = TOK_LE;
        } else {
            lex->current.type = TOK_LT;
        }
        lex->current.value = 0;
        lex->current.line  = tok_line;
        return;
    }
    if (c == '>') {
        lexer_advance(lex);
        if (lexer_peek(lex) == '=') {
            lexer_advance(lex);
            lex->current.type = TOK_GE;
        } else {
            lex->current.type = TOK_GT;
        }
        lex->current.value = 0;
        lex->current.line  = tok_line;
        return;
    }
    if (c == '&') {
        lexer_advance(lex);
        if (lexer_peek(lex) == '&') {
            lexer_advance(lex);
            lex->current.type = TOK_AND;
        } else {
            lex->current.type = TOK_EOF; /* bitwise & not supported */
        }
        lex->current.value = 0;
        lex->current.line  = tok_line;
        return;
    }
    if (c == '|') {
        lexer_advance(lex);
        if (lexer_peek(lex) == '|') {
            lexer_advance(lex);
            lex->current.type = TOK_OR;
        } else {
            lex->current.type = TOK_EOF;
        }
        lex->current.value = 0;
        lex->current.line  = tok_line;
        return;
    }

    /* Single-char tokens */
    TokenType tt = lookup_single(c);
    if (tt != TOK_EOF) {
        lexer_advance(lex);
        lex->current.type  = tt;
        lex->current.value = 0;
        lex->current.line  = tok_line;
        return;
    }

    /* Unknown character */
    lexer_advance(lex);
    lex->current.type  = TOK_EOF;
    lex->current.value = 0;
    lex->current.line  = tok_line;
}

/* ===== Initialize lexer with source string ===== */
void lexer_init(Lexer *lex, const char *src) {
    lex->src  = src;
    lex->pos  = 0;
    lex->line = 1;
    lexer_next(lex); /* prime first token */
}
