#include "fcc.h"
#include "stdio.h"
#include "string.h"
#include "ff.h"

#include "shell.h"
#include "shell_port.h"

/* Parser/Codegen API (lexer API is in fcc.h) */
int fcc_compile(Compiler *comp);
int cg_get_label_pos(int label_id);

/* ===== Source buffer ===== */
static char src_buf[FCC_SRC_MAX + 1u];
extern uint32_t fcc_code_buffer[FCC_CODE_MAX / sizeof(uint32_t)];
static uint32_t global_buffer[FCC_GLOBAL_MAX / sizeof(uint32_t)];

/* ===== Singleton compiler ===== */
static Compiler compiler;
static int (*compiled_main)(void);

/* ===== Path construction (same pattern as editor) ===== */

static int fcc_make_path(char *fullpath, size_t size, const char *user_path) {
    Shell *shell = shellGetCurrent();
    const char *cwd = "/";
    int length;

    if (shell != NULL) {
        cwd = shellGetPath(shell);
        if (cwd == NULL) cwd = "/";
    }

    if (user_path[0] == '/') {
        length = snprintf(fullpath, size, "0:%s", user_path);
    } else {
        size_t cwd_len = strlen(cwd);
        if (cwd_len > 0 && cwd[cwd_len - 1] == '/') {
            length = snprintf(fullpath, size, "0:%s%s", cwd, user_path);
        } else {
            length = snprintf(fullpath, size, "0:%s/%s", cwd, user_path);
        }
    }
    return length >= 0 && (size_t)length < size ? 0 : -1;
}

/* ===== File loading ===== */

static int fcc_load_file(const char *path) {
    char fullpath[512];
    FSIZE_t file_size;
    UINT bytes_read;
    FRESULT close_result;

    if (fcc_make_path(fullpath, sizeof(fullpath), path) != 0) {
        printf("fcc: file path is too long\r\n");
        return -1;
    }

    FIL file;
    FRESULT res = f_open(&file, fullpath, FA_OPEN_EXISTING | FA_READ);
    if (res != FR_OK) {
        printf("fcc: cannot open '%s' (code %d)\r\n", path, res);
        return -1;
    }

    file_size = f_size(&file);
    if (file_size > FCC_SRC_MAX) {
        printf("fcc: source file too large (max %d bytes)\r\n", FCC_SRC_MAX);
        (void)f_close(&file);
        return -1;
    }

    res = f_read(&file, src_buf, (UINT)file_size, &bytes_read);
    close_result = f_close(&file);

    if (res != FR_OK || close_result != FR_OK || bytes_read != file_size) {
        printf("fcc: read error (code %d)\r\n",
               res != FR_OK ? res : close_result);
        return -1;
    }

    for (UINT index = 0u; index < bytes_read; ++index) {
        if (src_buf[index] == '\0') {
            printf("fcc: source contains a NUL byte\r\n");
            return -1;
        }
    }
    src_buf[bytes_read] = '\0';
    return 0;
}

/* ===== Shell command ===== */

int fccCmd(int argc, char *argv[]) {
    compiled_main = NULL;
    if (argc != 2) {
        printf("Usage: fcc <source.c>\r\n");
        return -1;
    }

    /* Load source */
    if (fcc_load_file(argv[1]) != 0) {
        return -1;
    }

    /* Initialize compiler */
    memset(&compiler, 0, sizeof(compiler));
    compiler.code = fcc_code_buffer;
    compiler.code_capacity = sizeof(fcc_code_buffer);
    compiler.globals = global_buffer;
    compiler.global_base = (uint32_t)(uintptr_t)global_buffer;
    compiler.global_capacity = sizeof(global_buffer);
    if (envFunc == 0u)
        create_shell_env_varible();
    compiler.exec_func_addr = (uint32_t)envFunc;
    lexer_init(&compiler.lexer, src_buf);

    /* Compile */
    int result = fcc_compile(&compiler);
    if (result != 0) {
        printf("fcc: compilation failed\r\n");
        return -1;
    }

    /* Success — find main entry and print code location */
    uint32_t main_addr = 0u;
    for (int i = 0; i < compiler.func_count; i++) {
        if (strcmp(compiler.funcs[i].name, "main") == 0) {
            if (compiler.funcs[i].params != 0) {
                printf("fcc: main must take no arguments\r\n");
                return -1;
            }
            int pos = cg_get_label_pos(compiler.funcs[i].label_id);
            if (pos >= 0) {
                main_addr = (uint32_t)(uintptr_t)fcc_code_buffer + (uint32_t)pos;
            }
            break;
        }
    }
    if (main_addr == 0u) {
        printf("fcc: no main function\r\n");
        return -1;
    }
    compiled_main = (int (*)(void))(uintptr_t)main_addr;
    printf("fcc: OK  main=0x%08X  code=0x%08X  size=%u bytes\r\n",
           main_addr, (unsigned)(uintptr_t)fcc_code_buffer, compiler.code_size);
    printf("     run: fcc-run\r\n");

    return 0;
}

int fccRunCmd(int argc, char *argv[]) {
    (void)argv;
    if (argc != 1 || compiled_main == NULL) {
        printf("fcc: compile a source file before running\r\n");
        return -1;
    }

#if defined(__riscv)
    __asm__ volatile(".word 0x0000100f" ::: "memory"); /* fence.i */
#endif
    int result = compiled_main();
    printf("fcc: program returned %d\r\n", result);
    return result;
}
