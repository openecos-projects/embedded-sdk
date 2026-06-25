#include "fcc.h"
#include "stdio.h"

/* ===== RISC-V Opcodes ===== */
enum {
    OP_IMM  = 0x13,
    OP      = 0x33,
    LOAD    = 0x03,
    STORE   = 0x23,
    LUI     = 0x37,
    JALR    = 0x67,
    JAL     = 0x6F,
    BRANCH  = 0x63,
    AUIPC   = 0x17,
};

/* ===== funct3 / funct7 ===== */
enum {
    F3_ADD_SUB  = 0,   /* funct7=0x00 → ADD, funct7=0x20 → SUB */
    F3_LW       = 2,
    F3_SW       = 2,
    F3_JALR     = 0,
    F3_BEQ      = 0,
    F3_BNE      = 1,
    F3_BLT      = 4,
    F3_BGE      = 5,
    F7_NORMAL   = 0x00,
    F7_SUB      = 0x20,
    F7_MUL_DIV  = 0x01,  /* M extension */
};

/* ===== Instruction encoding helpers ===== */

static uint32_t enc_r(uint32_t funct7, uint32_t rs2, uint32_t rs1,
                       uint32_t funct3, uint32_t rd, uint32_t opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15)
         | (funct3 << 12) | (rd << 7) | opcode;
}

static uint32_t enc_i(uint32_t imm12, uint32_t rs1, uint32_t funct3,
                       uint32_t rd, uint32_t opcode) {
    return ((imm12 & 0xFFF) << 20) | (rs1 << 15)
         | (funct3 << 12) | (rd << 7) | opcode;
}

static uint32_t enc_s(uint32_t imm12, uint32_t rs2, uint32_t rs1,
                       uint32_t funct3, uint32_t opcode) {
    return ((imm12 & 0xFE0) << 20) | (rs2 << 20) | (rs1 << 15)
         | (funct3 << 12) | ((imm12 & 0x1F) << 7) | opcode;
}

static uint32_t enc_u(uint32_t imm20, uint32_t rd, uint32_t opcode) {
    return ((imm20 & 0xFFFFF) << 12) | (rd << 7) | opcode;
}

/* ===== Code buffer management ===== */

static Compiler *g_compiler;

void codegen_init(Compiler *comp) {
    g_compiler = comp;
    comp->code      = (uint32_t *)FCC_CODE_BASE;
    comp->code_size = 0;
}

static void emit(uint32_t instr) {
    if (g_compiler->code_size < FCC_CODE_MAX) {
        g_compiler->code[g_compiler->code_size / 4] = instr;
        g_compiler->code_size += 4;
    }
}

/* ===== Instruction emitters ===== */

/* ADDI rd, rs1, imm12 */
void cg_addi(int rd, int rs1, int imm12) {
    uint32_t instr = enc_i((uint32_t)imm12, (uint32_t)rs1, F3_ADD_SUB,
                           (uint32_t)rd, OP_IMM);
    emit(instr);
}

/* ADD rd, rs1, rs2 */
void cg_add(int rd, int rs1, int rs2) {
    uint32_t instr = enc_r(F7_NORMAL, (uint32_t)rs2, (uint32_t)rs1,
                           F3_ADD_SUB, (uint32_t)rd, OP);
    emit(instr);
}

/* SUB rd, rs1, rs2 */
void cg_sub(int rd, int rs1, int rs2) {
    uint32_t instr = enc_r(F7_SUB, (uint32_t)rs2, (uint32_t)rs1,
                           F3_ADD_SUB, (uint32_t)rd, OP);
    emit(instr);
}

/* MUL rd, rs1, rs2  (RV32M) */
void cg_mul(int rd, int rs1, int rs2) {
    uint32_t instr = enc_r(F7_MUL_DIV, (uint32_t)rs2, (uint32_t)rs1,
                           0, (uint32_t)rd, OP);
    emit(instr);
}

/* DIV rd, rs1, rs2  (RV32M) */
void cg_div(int rd, int rs1, int rs2) {
    uint32_t instr = enc_r(F7_MUL_DIV, (uint32_t)rs2, (uint32_t)rs1,
                           4, (uint32_t)rd, OP);
    emit(instr);
}

/* REM rd, rs1, rs2  (RV32M) */
void cg_rem(int rd, int rs1, int rs2) {
    uint32_t instr = enc_r(F7_MUL_DIV, (uint32_t)rs2, (uint32_t)rs1,
                           6, (uint32_t)rd, OP);
    emit(instr);
}

/* LW rd, offset(rs1) */
void cg_lw(int rd, int rs1, int offset) {
    uint32_t instr = enc_i((uint32_t)offset, (uint32_t)rs1, F3_LW,
                           (uint32_t)rd, LOAD);
    emit(instr);
}

/* SW rs2, offset(rs1) */
void cg_sw(int rs2, int rs1, int offset) {
    uint32_t instr = enc_s((uint32_t)offset, (uint32_t)rs2, (uint32_t)rs1,
                           F3_SW, STORE);
    emit(instr);
}

/* LUI rd, imm20 — load upper immediate */
void cg_lui(int rd, int imm20) {
    uint32_t instr = enc_u((uint32_t)imm20, (uint32_t)rd, LUI);
    emit(instr);
}

/* JALR rd, rs1, offset  —  rd=0, rs1=ra, offset=0  → ret */
void cg_jalr(int rd, int rs1, int offset) {
    uint32_t instr = enc_i((uint32_t)offset, (uint32_t)rs1, F3_JALR,
                           (uint32_t)rd, JALR);
    emit(instr);
}

/* JAL rd, offset  — jump and link */
void cg_jal(int rd, int offset) {
    /* J-type: imm[20|10:1|11|19:12] | rd | opcode */
    uint32_t imm = (uint32_t)offset;
    uint32_t instr = ((imm & 0x100000) << 11)       /* imm[20] → bit 31 */
                   | ((imm & 0x7FE)    << 20)        /* imm[10:1] → bits 30:21 */
                   | ((imm & 0x800)    <<  9)        /* imm[11] → bit 20 */
                   | (imm & 0xFF000)                  /* imm[19:12] → bits 19:12 */
                   | ((uint32_t)rd << 7)
                   | JAL;
    emit(instr);
}

/* Branch instructions */
void cg_beq(int rs1, int rs2, int offset) {
    uint32_t imm = (uint32_t)offset;
    uint32_t instr = ((imm & 0x1000) << 19)         /* imm[12] → bit 31 */
                   | ((imm & 0x1E)  <<  7)          /* imm[4:1] → bits 11:8 */
                   | ((imm & 0x7E0) << 20)          /* imm[10:5] → bits 30:25 */
                                                   | ((uint32_t)rs2 << 20)
                                                   | ((uint32_t)rs1 << 15)
                                                   | (F3_BEQ << 12)
                                                   | BRANCH;
    emit(instr);
}

void cg_bne(int rs1, int rs2, int offset) {
    uint32_t imm = (uint32_t)offset;
    uint32_t instr = ((imm & 0x1000) << 19)
                   | ((imm & 0x1E)  <<  7)
                   | ((imm & 0x7E0) << 20)
                   | ((uint32_t)rs2 << 20)
                   | ((uint32_t)rs1 << 15)
                   | (F3_BNE << 12)
                   | BRANCH;
    emit(instr);
}

void cg_blt(int rs1, int rs2, int offset) {
    uint32_t imm = (uint32_t)offset;
    uint32_t instr = ((imm & 0x1000) << 19)
                   | ((imm & 0x1E)  <<  7)
                   | ((imm & 0x7E0) << 20)
                   | ((uint32_t)rs2 << 20)
                   | ((uint32_t)rs1 << 15)
                   | (F3_BLT << 12)
                   | BRANCH;
    emit(instr);
}

void cg_bge(int rs1, int rs2, int offset) {
    uint32_t imm = (uint32_t)offset;
    uint32_t instr = ((imm & 0x1000) << 19)
                   | ((imm & 0x1E)  <<  7)
                   | ((imm & 0x7E0) << 20)
                   | ((uint32_t)rs2 << 20)
                   | ((uint32_t)rs1 << 15)
                   | (F3_BGE << 12)
                   | BRANCH;
    emit(instr);
}

/* ===== Compound operations ===== */

/* li rd, imm  — load immediate (pseudo-instruction) */
void cg_li(int rd, int imm) {
    if (imm >= -2048 && imm <= 2047) {
        /* Fits in 12-bit signed */
        cg_addi(rd, REG_ZERO, imm);
    } else {
        /* Need LUI + ADDI sequence */
        int upper = imm >> 12;
        int lower = imm & 0xFFF;
        /* Sign-extend correction */
        if (lower & 0x800) {
            upper = (upper + 1) & 0xFFFFF;
            lower = lower - 0x1000; /* lower is now negative */
        }
        if (upper != 0) {
            cg_lui(rd, upper);
            if (lower != 0) {
                cg_addi(rd, rd, lower);
            }
        } else {
            cg_addi(rd, REG_ZERO, lower);
        }
    }
}

/* Function prologue */
/* frame_bytes = 16 (ra+s0 slot) + local var bytes */
void cg_prologue(int frame_bytes) {
    /* sp -= frame_bytes;  save ra, s0;  s0 = sp (frame pointer for locals) */
    cg_addi(REG_SP, REG_SP, -frame_bytes);
    cg_sw(REG_RA, REG_SP, frame_bytes - 4);
    cg_sw(REG_S0, REG_SP, frame_bytes - 8);
    cg_addi(REG_S0, REG_SP, 0);
}

/* ===== Function prologue with parameter storage ===== */
/* Stores a0..a(N-1) into the first N local variable slots via s0 frame pointer.
 * Caller must already have allocated local vars at offsets 0,4,8,... for params. */
void cg_prologue_params(int frame_bytes, int param_count) {
    cg_addi(REG_SP, REG_SP, -frame_bytes);
    cg_sw(REG_RA, REG_SP, frame_bytes - 4);
    cg_sw(REG_S0, REG_SP, frame_bytes - 8);
    cg_addi(REG_S0, REG_SP, 0);  /* s0 = sp = frame pointer */
    /* Store incoming a-registers into local var slots (offsets 0, 4, 8, ...) */
    for (int i = 0; i < param_count; i++) {
        cg_sw(REG_A0 + i, REG_S0, i * 4);
    }
}

void cg_epilogue(int frame_bytes) {
    cg_lw(REG_S0, REG_SP, frame_bytes - 8);
    cg_lw(REG_RA, REG_SP, frame_bytes - 4);
    cg_addi(REG_SP, REG_SP, frame_bytes);
    cg_jalr(REG_ZERO, REG_RA, 0);  /* ret */
}

/* ===== Local variable access (frame-pointer-relative via s0) ===== */
/* Uses s0 (frame pointer) rather than sp, because sp changes during
 * expression stack evaluation.  s0 stays fixed at the bottom of the frame
 * for the whole function. */

/* Store value from t0 to local var at byte offset from s0 */
void cg_store_local(int offset) {
    cg_sw(REG_T0, REG_S0, offset);
}

/* Load local var at byte offset from s0 into rd */
void cg_load_local(int rd, int offset) {
    cg_lw(rd, REG_S0, offset);
}

/* ===== Global variable access (absolute address) ===== */

/* Store t0 to global at absolute address */
void cg_store_global(int addr) {
    cg_li(REG_T1, addr);
    cg_sw(REG_T0, REG_T1, 0);
}

/* Load global from absolute address into rd */
void cg_load_global(int rd, int addr) {
    cg_li(REG_T1, addr);
    cg_lw(rd, REG_T1, 0);
}

/* ===== Expression evaluation stack ===== */

/* Push an immediate value onto the expression stack */
void cg_push_imm(int imm) {
    cg_li(REG_T0, imm);
    cg_addi(REG_SP, REG_SP, -4);
    cg_sw(REG_T0, REG_SP, 0);
}

/* Pop the top of the expression stack into register rd */
void cg_pop(int rd) {
    cg_lw(rd, REG_SP, 0);
    cg_addi(REG_SP, REG_SP, 4);
}

/* Binary operator: pop two operands, compute, push result */
static void cg_binop(void (*op_func)(int, int, int), const char *op_name) {
    cg_pop(REG_T1);         /* right operand  */
    cg_pop(REG_T0);         /* left operand   */
    op_func(REG_T0, REG_T0, REG_T1);
    cg_addi(REG_SP, REG_SP, -4);
    cg_sw(REG_T0, REG_SP, 0);
}

void cg_add_op(void) { cg_binop(cg_add, "add"); }
void cg_sub_op(void) { cg_binop(cg_sub, "sub"); }
void cg_mul_op(void) { cg_binop(cg_mul, "mul"); }
void cg_div_op(void) { cg_binop(cg_div, "div"); }
void cg_rem_op(void) { cg_binop(cg_rem, "rem"); }

/* Unary minus: pop, negate, push */
void cg_neg_op(void) {
    cg_pop(REG_T0);
    cg_sub(REG_T0, REG_ZERO, REG_T0);
    cg_addi(REG_SP, REG_SP, -4);
    cg_sw(REG_T0, REG_SP, 0);
}

/* ===== SLT / SLTIU / XORI ===== */

/* SLT rd, rs1, rs2 */
void cg_slt(int rd, int rs1, int rs2) {
    uint32_t instr = enc_r(0, (uint32_t)rs2, (uint32_t)rs1, 2, (uint32_t)rd, OP);
    emit(instr);
}

/* SLTIU rd, rs1, imm  (set-less-than-immediate-unsigned) */
void cg_sltiu(int rd, int rs1, int imm) {
    uint32_t instr = enc_i((uint32_t)imm, (uint32_t)rs1, 3, (uint32_t)rd, OP_IMM);
    emit(instr);
}

/* XORI rd, rs1, imm */
void cg_xori(int rd, int rs1, int imm) {
    uint32_t instr = enc_i((uint32_t)imm, (uint32_t)rs1, 4, (uint32_t)rd, OP_IMM);
    emit(instr);
}

/* ===== Comparison operators (pop two, push 0 or 1) ===== */

/* lt:  left < right */
void cg_lt_op(void) {
    cg_pop(REG_T1); cg_pop(REG_T0);
    cg_slt(REG_T0, REG_T0, REG_T1);
    cg_addi(REG_SP, REG_SP, -4); cg_sw(REG_T0, REG_SP, 0);
}
/* ge:  left >= right  =  !(left < right) */
void cg_ge_op(void) {
    cg_pop(REG_T1); cg_pop(REG_T0);
    cg_slt(REG_T0, REG_T0, REG_T1);
    cg_xori(REG_T0, REG_T0, 1);
    cg_addi(REG_SP, REG_SP, -4); cg_sw(REG_T0, REG_SP, 0);
}
/* eq:  left == right */
void cg_eq_op(void) {
    cg_pop(REG_T1); cg_pop(REG_T0);
    cg_sub(REG_T0, REG_T0, REG_T1);
    cg_sltiu(REG_T0, REG_T0, 1);
    cg_addi(REG_SP, REG_SP, -4); cg_sw(REG_T0, REG_SP, 0);
}
/* ne:  left != right */
void cg_ne_op(void) {
    cg_pop(REG_T1); cg_pop(REG_T0);
    cg_sub(REG_T0, REG_T0, REG_T1);
    cg_slt(REG_T0, REG_ZERO, REG_T0);
    cg_addi(REG_SP, REG_SP, -4); cg_sw(REG_T0, REG_SP, 0);
}
/* gt:  left > right  =  right < left */
void cg_gt_op(void) {
    cg_pop(REG_T1); cg_pop(REG_T0);
    cg_slt(REG_T0, REG_T1, REG_T0);
    cg_addi(REG_SP, REG_SP, -4); cg_sw(REG_T0, REG_SP, 0);
}
/* le:  left <= right  =  !(right < left) */
void cg_le_op(void) {
    cg_pop(REG_T1); cg_pop(REG_T0);
    cg_slt(REG_T0, REG_T1, REG_T0);
    cg_xori(REG_T0, REG_T0, 1);
    cg_addi(REG_SP, REG_SP, -4); cg_sw(REG_T0, REG_SP, 0);
}

/* ===== Label / fixup system ===== */

#define MAX_LABELS 64

static int  label_pos[MAX_LABELS];   /* byte offset of label */
static int  fixup_pos[MAX_LABELS];   /* byte offset of branch instruction */
static int  fixup_kind[MAX_LABELS];  /* 0=BEQZ, 1=J, 2=CALL (same J-type patch) */
static int  label_count = 0;
static int  fixup_count = 0;

/* Reset label/fixup state for a new compilation */
void cg_reset_labels(void) {
    label_count = 0;
    fixup_count = 0;
}

int cg_get_label_pos(int label_id) {
    if (label_id < 0 || label_id >= label_count) return -1;
    return label_pos[label_id];
}

/* Record a label at the current code position */
int cg_label(void) {
    if (label_count >= MAX_LABELS) return -1;
    int id = label_count++;
    label_pos[id] = (int)g_compiler->code_size;
    return id;
}

/* Emit BEQ rd, x0, 0 (placeholder — patched later). Returns fixup id. */
int cg_emit_beqz(int rd) {
    if (fixup_count >= MAX_LABELS) return -1;
    int id = fixup_count++;
    fixup_pos[id]  = (int)g_compiler->code_size;
    fixup_kind[id] = 0;
    cg_beq(rd, REG_ZERO, 0);  /* offset will be patched */
    return id;
}

/* ===== Logic operators (short-circuit) ===== */

/* Unary NOT: !a — pop, push (t0 == 0) ? 1 : 0 */
void cg_not_op(void) {
    cg_pop(REG_T0);
    /* sltiu t0, t0, 1  →  t0 = (t0 < 1) ? 1 : 0   i.e.  t0 = !t0 */
    cg_sltiu(REG_T0, REG_T0, 1);
    cg_addi(REG_SP, REG_SP, -4);
    cg_sw(REG_T0, REG_SP, 0);
}

/* Short-circuit AND begin: pop a, beqz t0 → skip_to_false.
 * Returns fixup id for the skip. */
int cg_and_begin(void) {
    cg_pop(REG_T0);
    return cg_emit_beqz(REG_T0);
}

/* Short-circuit OR begin: pop a, beqz t0 → eval_b.
 * Returns fixup id for eval_b. */
int cg_or_begin(void) {
    cg_pop(REG_T0);
    return cg_emit_beqz(REG_T0);
}

/* Emit JAL x0, 0 (placeholder). Returns fixup id. */
int cg_emit_j(void) {
    if (fixup_count >= MAX_LABELS) return -1;
    int id = fixup_count++;
    fixup_pos[id]  = (int)g_compiler->code_size;
    fixup_kind[id] = 1;
    cg_jal(REG_ZERO, 0);  /* offset will be patched */
    return id;
}

/* ===== Function call fixup (forward calls) ===== */

/* Emit JAL ra, 0 (placeholder for forward function call).
 * Same J-type as cg_emit_j, but rd=ra to save return address.
 * Returns fixup id. */
int cg_emit_call(void) {
    if (fixup_count >= MAX_LABELS) return -1;
    int id = fixup_count++;
    fixup_pos[id]  = (int)g_compiler->code_size;
    fixup_kind[id] = 1;  /* J-type patch logic, same as J */
    cg_jal(REG_RA, 0);   /* JAL ra, 0 — call, save return address */
    return id;
}

/* Emit a backward function call to a known label.
 * JAL ra, offset — saves return address in ra. */
void cg_emit_call_back(int label_id) {
    if (label_id < 0 || label_id >= label_count) return;
    int target = label_pos[label_id];
    int offset = target - (int)g_compiler->code_size;
    cg_jal(REG_RA, offset);
}

/* Indirect function call via address in register.
 * JALR ra, rs, 0 — saves return address in ra. */
void cg_call_indirect(int rs) {
    cg_jalr(REG_RA, rs, 0);
}

/* Patch a previously emitted branch/jump to point to the current position */
void cg_patch(int fixup_id) {
    if (fixup_id < 0 || fixup_id >= fixup_count) return;
    int branch_pos = fixup_pos[fixup_id];
    int target_pos = (int)g_compiler->code_size;
    int offset = target_pos - branch_pos;

    /* Read the existing instruction, patch the offset */
    uint32_t old = g_compiler->code[branch_pos / 4];
    uint32_t patched;

    if (fixup_kind[fixup_id] == 0) {
        /* B-type: branch */
        uint32_t imm = (uint32_t)offset;
        /* Clear old immediate bits, set new ones */
        patched = old & 0x01FFF07F;  /* keep opcode, rd, rs1, rs2, funct3 */
        patched |= ((imm & 0x1000) << 19)     /* imm[12] */
                |  ((imm & 0x7E0)  << 20)     /* imm[10:5] */
                |  ((imm & 0x1E)   << 7)      /* imm[4:1] */
                |  ((imm & 0x800)  >> 4);     /* imm[11] */
    } else {
        /* J-type: jal / call — patch the immediate, preserve opcode+rd */
        uint32_t imm = (uint32_t)offset;
        patched = old & 0x00000FFF;  /* keep opcode (bits 6:0) + rd (bits 11:7) */
        patched |= ((imm & 0x100000) << 11)     /* imm[20] → bit 31 */
                |  ((imm & 0x7FE)    << 20)     /* imm[10:1] → bits 30:21 */
                |  ((imm & 0x800)    <<  9)     /* imm[11] → bit 20 */
                |  (imm & 0xFF000);              /* imm[19:12] → bits 19:12 */
    }
    g_compiler->code[branch_pos / 4] = patched;
}

/* Emit an unconditional jump to a backward label.
 * No fixup needed since the target is known. */
void cg_emit_jback(int label_id) {
    if (label_id < 0 || label_id >= label_count) return;
    int target = label_pos[label_id];
    int offset = target - (int)g_compiler->code_size;
    cg_jal(REG_ZERO, offset);
}

/* Emit BEQ rd, x0, offset to a backward label */
void cg_emit_beqz_back(int rd, int label_id) {
    if (label_id < 0 || label_id >= label_count) return;
    int target = label_pos[label_id];
    int offset = target - (int)g_compiler->code_size;
    cg_beq(rd, REG_ZERO, offset);
}
