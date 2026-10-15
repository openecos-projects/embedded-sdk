#include "fcc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

extern int fcc_compile(Compiler *compiler);
extern int cg_get_label_pos(int label_id);
extern void codegen_init(Compiler *compiler);
extern void cg_beq(int rs1, int rs2, int offset);

static Compiler compiler;
static uint32_t code[FCC_CODE_MAX / sizeof(uint32_t) + 1u];
static uint32_t globals[FCC_GLOBAL_MAX / sizeof(uint32_t)];

static int compile(const char *source, uint32_t capacity)
{
    memset(&compiler, 0, sizeof(compiler));
    memset(code, 0, sizeof(code));
    code[capacity / sizeof(uint32_t)] = 0x12345678u;
    compiler.code = code;
    compiler.code_capacity = capacity;
    compiler.globals = globals;
    compiler.global_base = 0x40000000u;
    compiler.global_capacity = sizeof(globals);
    lexer_init(&compiler.lexer, source);
    return fcc_compile(&compiler);
}

int main(void)
{
    char long_loop[16000];

    memset(&compiler, 0, sizeof(compiler));
    compiler.code = code;
    compiler.code_capacity = sizeof(code);
    codegen_init(&compiler);
    cg_beq(REG_T0, REG_ZERO, 2048);
    assert((code[0] & 0x80u) != 0u);
    assert(compiler.error == 0);

    assert(compile("int main(){return 7;}", FCC_CODE_MAX) == 0);
    assert(compiler.code_size > 0u);
    assert(compiler.func_count == 1);
    assert(cg_get_label_pos(compiler.funcs[0].label_id) == 0);

    assert(compile("int value=5;int main(){return value;}", FCC_CODE_MAX) == 0);
    assert(globals[0] == 5u);
    assert(cg_get_label_pos(compiler.funcs[0].label_id) == 0);
    assert(compile("int value=-5;int main(){return value;}", FCC_CODE_MAX) == 0);
    assert(globals[0] == (uint32_t)-5);

    assert(compile("int counter=1;"
                   "int main(){counter=counter+1;"
                   "if(counter==2){return twice(21);}return 0;}"
                   "int twice(int x){return x*2;}", FCC_CODE_MAX) == 0);
    assert(globals[0] == 1u);
    assert(compile("int main(){int x;x=0;while(x<3){x=x+1;}return x;}",
                   FCC_CODE_MAX) == 0);

    assert(compile("int main(){return twice(21);}"
                   "int twice(int x){return x*2;}", FCC_CODE_MAX) == 0);
    assert(compiler.func_count == 2);
    assert(compiler.funcs[1].label_id >= 0);
    assert(compile("int main(){return twice(21);}"
                   "int twice(){return 1;}", FCC_CODE_MAX) != 0);
    assert(compile("int twice(int x){return x;}"
                   "int main(){return twice();}", FCC_CODE_MAX) != 0);
    assert(compile("int main(){return exec_func(1);}", FCC_CODE_MAX) != 0);

    assert(compile("int main(){return 7;return 8;}", FCC_CODE_MAX) == 0);
    assert(compiler.code_size > 0u);

    assert(compile("int main(){return #;}", FCC_CODE_MAX) != 0);
    assert(compile("int main(){return 999999999999;}", FCC_CODE_MAX) != 0);

    assert(compile("int main(){return 7;}", 16u) != 0);
    assert(compiler.code_size <= 16u);
    assert(code[4] == 0x12345678u);

    strcpy(long_loop, "int main(){int x;x=0;while(x<1){");
    for (int index = 0; index < 400; ++index)
        strcat(long_loop, "x=x+1;");
    strcat(long_loop, "}return x;}");
    assert(compile(long_loop, FCC_CODE_MAX) != 0);
    assert(code[FCC_CODE_MAX / sizeof(uint32_t)] == 0x12345678u);

    return 0;
}
