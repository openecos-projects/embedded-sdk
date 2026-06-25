#include "fcc.h"
#include "stdio.h"
#include "string.h"

/* ===== Codegen API ===== */
void codegen_init(Compiler *comp);
void cg_reset_labels(void);
void cg_prologue(int frame_bytes);
void cg_prologue_params(int frame_bytes, int param_count);
void cg_epilogue(int frame_bytes);
void cg_pop(int rd);
void cg_push_imm(int imm);
void cg_add_op(void);  void cg_sub_op(void);
void cg_mul_op(void);  void cg_div_op(void);  void cg_rem_op(void);
void cg_neg_op(void);
void cg_not_op(void);
int  cg_and_begin(void);
int  cg_or_begin(void);
void cg_store_local(int offset);
void cg_load_local(int rd, int offset);
void cg_store_global(int addr);
void cg_load_global(int rd, int addr);
void cg_addi(int rd, int rs1, int imm);
void cg_sw(int rs2, int rs1, int offset);
void cg_li(int rd, int imm);
void cg_lt_op(void); void cg_le_op(void); void cg_gt_op(void);
void cg_ge_op(void); void cg_eq_op(void); void cg_ne_op(void);
int  cg_label(void);
int  cg_emit_beqz(int rd);
int  cg_emit_j(void);
int  cg_emit_call(void);
void cg_emit_call_back(int label_id);
void cg_call_indirect(int rs);
void cg_patch(int fixup_id);
void cg_emit_jback(int label_id);

/* ===== Symbol table ===== */

static VarInfo *sym_add(Compiler *comp, const char *name, int is_global) {
    if (comp->var_count >= MAX_VARS) return NULL;
    VarInfo *v = &comp->vars[comp->var_count++];
    strncpy(v->name, name, 63);
    v->name[63] = '\0';
    v->is_global = is_global;
    v->offset = 0;
    return v;
}

static VarInfo *sym_lookup(Compiler *comp, const char *name) {
    for (int i = comp->var_count - 1; i >= 0; i--) {
        if (strcmp(comp->vars[i].name, name) == 0)
            return &comp->vars[i];
    }
    return NULL;
}

/* ===== Function table ===== */

/* Add a function to the table (forward declaration if label_id=-1).
 * Returns the FuncInfo pointer, or NULL if table full or duplicate. */
static FuncInfo *func_add(Compiler *comp, const char *name) {
    /* Check for duplicate */
    for (int i = 0; i < comp->func_count; i++) {
        if (strcmp(comp->funcs[i].name, name) == 0) {
            /* If already defined (label_id >= 0), it's a real duplicate */
            if (comp->funcs[i].label_id >= 0) return NULL;
            /* Forward-declared but not yet defined — return existing entry */
            return &comp->funcs[i];
        }
    }
    if (comp->func_count >= MAX_FUNCS) return NULL;
    FuncInfo *f = &comp->funcs[comp->func_count++];
    strncpy(f->name, name, 63);
    f->name[63] = '\0';
    f->label_id = -1;
    f->params    = 0;
    f->fwd_count = 0;
    return f;
}

/* Look up a function by name. Returns NULL if not found. */
static FuncInfo *func_lookup(Compiler *comp, const char *name) {
    for (int i = 0; i < comp->func_count; i++) {
        if (strcmp(comp->funcs[i].name, name) == 0)
            return &comp->funcs[i];
    }
    return NULL;
}

/* Mark a function as defined (records its code label) and patch forward calls. */
static int func_define(Compiler *comp, const char *name, int label_id, int params) {
    FuncInfo *f = func_lookup(comp, name);
    if (f && f->label_id >= 0) {
        /* Already defined — duplicate */
        return -1;
    }
    if (f == NULL) {
        f = func_add(comp, name);
        if (f == NULL) return -1;
    }
    f->label_id = label_id;
    f->params   = params;

    /* Patch all forward calls to this function */
    for (int i = 0; i < f->fwd_count; i++) {
        cg_patch(f->fwd_fixups[i]);
    }
    f->fwd_count = 0;
    return 0;
}

/* Record a forward call fixup for a function */
static int func_add_fwd_fixup(FuncInfo *f, int fixup_id) {
    if (f->fwd_count >= MAX_FWD_FIXUPS) return -1;
    f->fwd_fixups[f->fwd_count++] = fixup_id;
    return 0;
}

static char *tok_name(Compiler *comp) { return comp->lexer.current.name; }

/* ===== Helpers ===== */

static void parse_error(Compiler *comp, const char *msg) {
    printf("fcc:%d: error: %s\r\n", comp->lexer.current.line, msg);
    comp->error = 1;
}

static void expect(Compiler *comp, TokenType type, const char *what) {
    if (!comp->error && comp->lexer.current.type != type) {
        char buf[80];
        snprintf(buf, sizeof(buf), "expected %s", what);
        parse_error(comp, buf);
    }
    if (!comp->error) lexer_next(&comp->lexer);
}

static void advance(Compiler *comp) {
    if (!comp->error) lexer_next(&comp->lexer);
}

/* ===== Forward declarations ===== */
static void parse_or(Compiler *comp);
static void parse_and(Compiler *comp);
static void parse_cmp(Compiler *comp);
static void parse_stmt_loop(Compiler *comp);
static void parse_assign(Compiler *comp, VarInfo *v);

/* ===== Expression parser ===== */

static void parse_call(Compiler *comp, const char *name);

static void parse_primary(Compiler *comp) {
    if (comp->lexer.current.type == TOK_NUMBER) {
        int val = comp->lexer.current.value;
        advance(comp);
        cg_push_imm(val);
        return;
    }
    if (comp->lexer.current.type == TOK_IDENT) {
        /* Copy name NOW — tok_name returns a pointer into lex->current.name
         * which will be overwritten by the next lexer_next() call inside
         * advance() or any nested expression parsing. */
        char name_buf[64];
        strncpy(name_buf, tok_name(comp), 63);
        name_buf[63] = '\0';
        char *name = name_buf;

        /* Check if this is a function call: ident followed by '(' */
        /* Peek ahead — we need to check the next token type.
         * Since we don't have a peek, we check the source.
         * Actually, we can look ahead: if we advance past the ident,
         * and the current token is '(', it's a call. But that changes
         * lexer state. Better: check if name is known as a function
         * (built-in or user-defined) and the next token is '('. */
        /* Strategy: advance past the ident, check if next is '('.
         * If yes, it's a call. If no, rewind is not possible with
         * our simple lexer — instead, handle both paths:
         *
         * Path A (call): advance, parse_call handles '('
         * Path B (variable): don't advance yet, load the variable
         *
         * We can distinguish by checking the source character after
         * the identifier. The lexer's `pos` points to the character
         * after the identifier name and any whitespace.
         */

        /* Simple approach: if the source has '(' after the ident,
         * treat as function call (parse_call handles forward declarations).
         * Otherwise treat as variable. */
        int is_func = 0;
        if (strcmp(name, "exec_func") == 0) {
            is_func = 1;
        } else {
            FuncInfo *f = func_lookup(comp, name);
            if (f != NULL) is_func = 1;
        }

        /* Determine if next token is '(' */
        int next_is_call = 0;
        {
            Lexer *lex = &comp->lexer;
            int check_pos = lex->pos;
            const char *src = lex->src;
            int src_len = (int)strlen(src);
            while (check_pos < src_len
                   && (src[check_pos] == ' ' || src[check_pos] == '\t'
                       || src[check_pos] == '\r' || src[check_pos] == '\n'))
                check_pos++;
            if (check_pos < src_len && src[check_pos] == '(')
                next_is_call = 1;
        }

        if (is_func && next_is_call) {
            /* Function call — known function (or exec_func) followed by '(' */
            advance(comp);
            parse_call(comp, name);
            return;
        }

        /* Not a function call — treat as variable */
        VarInfo *v = sym_lookup(comp, name);
        if (v == NULL) { parse_error(comp, "undefined variable"); return; }
        advance(comp);
        if (v->is_global) cg_load_global(REG_T0, v->offset);
        else              cg_load_local(REG_T0, v->offset);
        cg_addi(REG_SP, REG_SP, -4);
        cg_sw(REG_T0, REG_SP, 0);
        return;
    }
    if (comp->lexer.current.type == TOK_LPAREN) {
        advance(comp);
        parse_or(comp);
        expect(comp, TOK_RPAREN, "')'");
        return;
    }
    parse_error(comp, "expected expression");
}

/* parse_call — compile a function call: ident(args...)
 * 'name' is the function name, lexer is past the identifier. */
static void parse_call(Compiler *comp, const char *name) {
    int is_exec_func = (strcmp(name, "exec_func") == 0);

    /* We expect the current token to be '(' — the identifier was
     * already consumed by parse_primary. */
    if (comp->lexer.current.type == TOK_LPAREN) {
        advance(comp); /* skip '(' */
    } else {
        /* This shouldn't happen since we checked before calling,
         * but handle gracefully. */
        parse_error(comp, "expected '(' for function call");
        return;
    }

    /* Parse argument list */
    int arg_count = 0;
    if (comp->lexer.current.type != TOK_RPAREN) {
        do {
            if (arg_count >= 8) {
                parse_error(comp, "too many arguments (max 8)");
                /* Skip remaining args to recover */
                while (!comp->error
                       && comp->lexer.current.type != TOK_RPAREN
                       && comp->lexer.current.type != TOK_EOF)
                    advance(comp);
                break;
            }
            parse_or(comp);
            arg_count++;
        } while (comp->lexer.current.type == TOK_COMMA && (advance(comp), 1));
    }
    expect(comp, TOK_RPAREN, "')'");


    /* Pop arguments from expression stack into a-registers (reverse order).
     * Expression stack: arg0 at bottom, arg(N-1) at top.
     * We need: a0 = arg0, a1 = arg1, ...
     * Pop from top: arg(N-1) → a(N-1), ..., arg0 → a0 */
    for (int i = arg_count - 1; i >= 0; i--) {
        cg_pop(REG_A0 + i);
    }

    if (is_exec_func) {
        /* Built-in exec_func: load its address and call indirectly */
        cg_li(REG_T1, (int)comp->exec_func_addr);
        cg_call_indirect(REG_T1);
    } else {
        FuncInfo *f = func_lookup(comp, name);
        if (f == NULL) {
            /* Not yet seen — add as forward declaration */
            f = func_add(comp, name);
            if (f == NULL) {
                parse_error(comp, "too many functions");
                return;
            }
            /* Emit forward call with fixup */
            int fixup_id = cg_emit_call();
            if (fixup_id < 0) {
                parse_error(comp, "too many fixups");
                return;
            }
            if (func_add_fwd_fixup(f, fixup_id) < 0) {
                parse_error(comp, "too many forward calls");
                return;
            }
            /* Remember param count for this forward declaration */
            f->params = arg_count;
        } else if (f->label_id >= 0) {
            /* Already defined — backward call */
            cg_emit_call_back(f->label_id);
        } else {
            /* Forward-declared but not yet defined — add another fixup */
            int fixup_id = cg_emit_call();
            if (fixup_id < 0) {
                parse_error(comp, "too many fixups");
                return;
            }
            if (func_add_fwd_fixup(f, fixup_id) < 0) {
                parse_error(comp, "too many forward calls");
                return;
            }
        }
    }

    /* Push return value (a0) onto expression stack */
    cg_addi(REG_SP, REG_SP, -4);
    cg_sw(REG_A0, REG_SP, 0);
}

static void parse_unary(Compiler *comp) {
    if (comp->lexer.current.type == TOK_NOT) {
        advance(comp);
        parse_unary(comp);
        cg_not_op();
        return;
    }
    if (comp->lexer.current.type == TOK_MINUS) {
        advance(comp);
        parse_unary(comp);
        cg_neg_op();
        return;
    }
    parse_primary(comp);
}

static void parse_mul(Compiler *comp) {
    parse_unary(comp);
    while (!comp->error
           && (comp->lexer.current.type == TOK_STAR
               || comp->lexer.current.type == TOK_SLASH
               || comp->lexer.current.type == TOK_PERCENT)) {
        TokenType op = comp->lexer.current.type;
        advance(comp);
        parse_unary(comp);
        if      (op == TOK_STAR)    cg_mul_op();
        else if (op == TOK_SLASH)   cg_div_op();
        else                        cg_rem_op();
    }
}

static void parse_add(Compiler *comp) {
    parse_mul(comp);
    while (!comp->error
           && (comp->lexer.current.type == TOK_PLUS
               || comp->lexer.current.type == TOK_MINUS)) {
        TokenType op = comp->lexer.current.type;
        advance(comp);
        parse_mul(comp);
        if (op == TOK_PLUS) cg_add_op();
        else                cg_sub_op();
    }
}

/* ===== Logic OR: a || b  (short-circuit) ===== */
static void parse_or(Compiler *comp) {
    parse_and(comp);
    while (!comp->error && comp->lexer.current.type == TOK_OR) {
        advance(comp);                            /* skip '||' */
        int fix_eval = cg_or_begin();             /* pop a, beqz → eval b */
        cg_push_imm(1);                           /* a != 0 → push 1 */
        int fix_jmp  = cg_emit_j();               /* jump over eval_b */
        cg_patch(fix_eval);                       /* here: eval b (a == 0) */
        parse_and(comp);                          /* evaluate b */
        cg_patch(fix_jmp);                        /* merge */
    }
}

/* ===== Logic AND: a && b  (short-circuit) ===== */
static void parse_and(Compiler *comp) {
    parse_cmp(comp);
    while (!comp->error && comp->lexer.current.type == TOK_AND) {
        advance(comp);                            /* skip '&&' */
        int fix_skip = cg_and_begin();            /* pop a, beqz → false */
        parse_cmp(comp);                          /* evaluate b */
        int fix_jmp  = cg_emit_j();               /* jump over false path */
        cg_patch(fix_skip);                       /* false path: push 0 */
        cg_push_imm(0);
        cg_patch(fix_jmp);                        /* merge */
    }
}

static void parse_cmp(Compiler *comp) {
    parse_add(comp);
    while (!comp->error
           && (comp->lexer.current.type == TOK_EQ
               || comp->lexer.current.type == TOK_NE
               || comp->lexer.current.type == TOK_LT
               || comp->lexer.current.type == TOK_GT
               || comp->lexer.current.type == TOK_LE
               || comp->lexer.current.type == TOK_GE)) {
        TokenType op = comp->lexer.current.type;
        advance(comp);
        parse_add(comp);
        switch (op) {
            case TOK_EQ: cg_eq_op(); break;
            case TOK_NE: cg_ne_op(); break;
            case TOK_LT: cg_lt_op(); break;
            case TOK_GT: cg_gt_op(); break;
            case TOK_LE: cg_le_op(); break;
            case TOK_GE: cg_ge_op(); break;
            default: break;
        }
    }
}

/* ===== Control flow ===== */

static void parse_block(Compiler *comp) {
    expect(comp, TOK_LBRACE, "'{'");
    parse_stmt_loop(comp);
    expect(comp, TOK_RBRACE, "'}'");
}

static void parse_if(Compiler *comp) {
    advance(comp);
    expect(comp, TOK_LPAREN, "'('");
    parse_or(comp);
    expect(comp, TOK_RPAREN, "')'");
    cg_pop(REG_T0);
    int fix_else = cg_emit_beqz(REG_T0);
    parse_block(comp);
    if (comp->lexer.current.type == TOK_ELSE) {
        advance(comp);
        int fix_end = cg_emit_j();
        cg_patch(fix_else);
        parse_block(comp);
        cg_patch(fix_end);
    } else {
        cg_patch(fix_else);
    }
}

static void parse_while(Compiler *comp) {
    advance(comp);
    int start = cg_label();
    expect(comp, TOK_LPAREN, "'('");
    parse_or(comp);
    expect(comp, TOK_RPAREN, "')'");
    cg_pop(REG_T0);
    int fix_end = cg_emit_beqz(REG_T0);
    parse_block(comp);
    cg_emit_jback(start);
    cg_patch(fix_end);
}

/* ===== Statement loop ===== */

static void parse_stmt_loop(Compiler *comp) {
    while (!comp->error
           && comp->lexer.current.type != TOK_RBRACE) {
        if (comp->lexer.current.type == TOK_IF) {
            parse_if(comp);
        } else if (comp->lexer.current.type == TOK_WHILE) {
            parse_while(comp);
        } else if (comp->lexer.current.type == TOK_RETURN) {
            advance(comp);
            parse_or(comp);
            cg_pop(REG_A0);
            expect(comp, TOK_SEMI, "';'");
        } else if (comp->lexer.current.type == TOK_IDENT) {
            /* Could be assignment or function call */
            /* Copy name to avoid clobbering by nested lexer_next */
            char name_buf[64];
            strncpy(name_buf, tok_name(comp), 63);
            name_buf[63] = '\0';
            char *name = name_buf;

            /* Check if this is a function call statement (result discarded)
             * or an assignment. Look ahead for '(' or '='. */
            Lexer *lex = &comp->lexer;
            int check_pos = lex->pos;
            const char *src = lex->src;
            int src_len = (int)strlen(src);
            while (check_pos < src_len
                   && (src[check_pos] == ' ' || src[check_pos] == '\t'
                       || src[check_pos] == '\r' || src[check_pos] == '\n'))
                check_pos++;

            int is_func = 0;
            if (strcmp(name, "exec_func") == 0) {
                is_func = 1;
            } else {
                FuncInfo *f = func_lookup(comp, name);
                if (f != NULL) is_func = 1;
            }

            if (is_func && check_pos < src_len && src[check_pos] == '(') {
                /* Function call as statement (result discarded) */
                advance(comp); /* consume the identifier */
                parse_call(comp, name);
                /* Discard the return value from the expression stack */
                cg_pop(REG_T0);
                expect(comp, TOK_SEMI, "';'");
            } else {
                /* Variable assignment */
                VarInfo *v = sym_lookup(comp, name);
                if (v == NULL) {
                    parse_error(comp, "undefined variable");
                } else {
                    advance(comp);
                    parse_assign(comp, v);
                }
            }
        } else {
            parse_error(comp, "expected statement");
            break;
        }
    }
}

static void parse_assign(Compiler *comp, VarInfo *v) {
    expect(comp, TOK_ASSIGN, "'='");
    parse_or(comp);
    cg_pop(REG_T0);
    if (v->is_global)
        cg_store_global(v->offset);
    else
        cg_store_local(v->offset);
    expect(comp, TOK_SEMI, "';'");
}

/* ===== Local var declaration ===== */

static void parse_local_decl(Compiler *comp) {
    if (comp->lexer.current.type != TOK_IDENT) {
        parse_error(comp, "expected variable name");
        return;
    }
    char *name = tok_name(comp);
    for (int i = comp->local_base; i < comp->var_count; i++) {
        if (strcmp(comp->vars[i].name, name) == 0) {
            parse_error(comp, "duplicate variable");
            return;
        }
    }
    advance(comp);

    VarInfo *v = sym_add(comp, name, 0);
    v->offset = comp->local_bytes;
    comp->local_bytes += 4;

    if (comp->lexer.current.type == TOK_ASSIGN) {
        parse_error(comp, "initializer not supported in declaration");
        return;
    }
    expect(comp, TOK_SEMI, "';'");
}

/* ===== Function body ===== */

static void parse_function_body(Compiler *comp, const char *name) {
    expect(comp, TOK_LPAREN, "'('");

    /* Parse optional parameter list: int name, int name, ...
     * Parameters become local variables allocated at offsets 0, 4, 8, ...
     * The prologue stores a0..a(N-1) into these slots. */
    int param_count = 0;
    comp->local_base = comp->var_count;
    comp->local_bytes = 0;

    if (comp->lexer.current.type != TOK_RPAREN) {
        do {
            if (comp->lexer.current.type != TOK_INT) {
                parse_error(comp, "expected 'int' in parameter list");
                return;
            }
            advance(comp); /* skip 'int' */

            if (comp->lexer.current.type != TOK_IDENT) {
                parse_error(comp, "expected parameter name");
                return;
            }

            char *pname = tok_name(comp);

            /* Check for duplicate parameter name */
            for (int i = comp->local_base; i < comp->var_count; i++) {
                if (strcmp(comp->vars[i].name, pname) == 0) {
                    parse_error(comp, "duplicate parameter");
                    return;
                }
            }
            advance(comp);

            if (param_count >= 8) {
                parse_error(comp, "too many parameters (max 8)");
                return;
            }

            VarInfo *v = sym_add(comp, pname, 0);
            v->offset = comp->local_bytes;
            comp->local_bytes += 4;
            param_count++;

        } while (comp->lexer.current.type == TOK_COMMA && (advance(comp), 1));
    }

    expect(comp, TOK_RPAREN, "')'");
    expect(comp, TOK_LBRACE, "'{'");

    comp->current_func_param_count = param_count;

    /* Record function definition before parsing body (for recursion support) */
    int func_label = cg_label();
    if (func_define(comp, name, func_label, param_count) < 0) {
        parse_error(comp, "duplicate function");
        return;
    }

    /* Parse local variable declarations (int x; ...) */
    while (!comp->error
           && comp->lexer.current.type == TOK_INT) {
        advance(comp);
        parse_local_decl(comp);
    }

    int frame = 16 + comp->local_bytes;
    cg_prologue_params(frame, param_count);

    parse_stmt_loop(comp);

    cg_epilogue(frame);
    comp->var_count = comp->local_base;

    expect(comp, TOK_RBRACE, "'}'");
}

/* ===== Top-level parser ===== */

static void parse_program(Compiler *comp) {
    while (!comp->error && comp->lexer.current.type != TOK_EOF) {
        if (comp->lexer.current.type != TOK_INT) {
            parse_error(comp, "expected 'int' at top level");
            break;
        }
        advance(comp);

        if (comp->lexer.current.type != TOK_IDENT) {
            parse_error(comp, "expected identifier");
            break;
        }
        char name[64];
        strncpy(name, tok_name(comp), 63);
        name[63] = '\0';
        advance(comp);

        if (comp->lexer.current.type == TOK_LPAREN) {
            /* Function definition */
            /* Pre-register with forward declaration (for recursion) */
            FuncInfo *existing = func_lookup(comp, name);
            if (existing && existing->label_id >= 0) {
                parse_error(comp, "duplicate function");
                /* Skip to recover */
                while (!comp->error
                       && comp->lexer.current.type != TOK_RBRACE
                       && comp->lexer.current.type != TOK_EOF)
                    advance(comp);
                if (comp->lexer.current.type == TOK_RBRACE) advance(comp);
                continue;
            }
            if (existing == NULL) {
                if (func_add(comp, name) == NULL) {
                    parse_error(comp, "too many functions");
                    break;
                }
            }
            parse_function_body(comp, name);
        } else {
            /* Global variable */
            if (sym_lookup(comp, name)) {
                parse_error(comp, "duplicate global variable");
                break;
            }
            VarInfo *v = sym_add(comp, name, 1);
            v->offset = FCC_GLOBAL_BASE + comp->next_global;
            comp->next_global += 4;

            if (comp->lexer.current.type == TOK_ASSIGN) {
                advance(comp);
                if (comp->lexer.current.type != TOK_NUMBER) {
                    parse_error(comp, "global init must be constant");
                    break;
                }
                cg_li(REG_T0, comp->lexer.current.value);
                cg_store_global(v->offset);
                advance(comp);
            }
            expect(comp, TOK_SEMI, "';'");
        }
    }

    /* Check that all forward-declared functions have been defined */
    if (!comp->error) {
        for (int i = 0; i < comp->func_count; i++) {
            if (comp->funcs[i].label_id < 0) {
                printf("fcc: error: undefined function '%s'\r\n",
                       comp->funcs[i].name);
                comp->error = 1;
            }
        }
    }
}

int fcc_compile(Compiler *comp) {
    codegen_init(comp);
    cg_reset_labels();
    comp->error       = 0;
    comp->var_count   = 0;
    comp->func_count  = 0;
    comp->next_global = 0;

    parse_program(comp);

    return comp->error ? -1 : 0;
}
