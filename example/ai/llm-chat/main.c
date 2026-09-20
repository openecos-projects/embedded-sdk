#include "ecos/bsp/console.h"
#include "ecos/driver/rcu.h"
#include "ecos/heap.h"
#include "ecos/log.h"

#include <stdint.h>

#include "runq.h"

#define LOG_TAG "llm-chat"

/* 可调旋钮：温度越大越发散（0 为贪心），top_p 截断低概率尾巴 */
#define LLM_TEMPERATURE      0.8f
#define LLM_TOP_P            0.9f
#define LLM_GEN_STEPS        160
#define LLM_RNG_SEED         42ull
#define LLM_PROMPT_MAX_CHARS 200u

/* CLINT mtime（5 MHz）是板上唯一可信的计时基准：经秒表实测校验，
 * Timer0 计数频率随时钟域变化，不能当墙上时钟使用。 */
#define MTIME_LOW_ADDR  ((uintptr_t)0x0200BFF8u)
#define MTIME_HIGH_ADDR ((uintptr_t)0x0200BFFCu)
#define MTIME_HZ        5000000u

/* 由 CMake 生成的 llm_data.S 经 .incbin 链接进 Flash（XIP 直接读） */
extern const uint8_t llm_model_start[];
extern const uint8_t llm_tokenizer_start[];

/* CPU/AXI/AHB 切到 PLL 提频加速推理；APB 保持 20 MHz，串口波特率不变。
 * 想继续加压可改 N 与分频（Fvco 限 500~1200 MHz），超出已验证组合需上板确认。 */
static const ecos_rcu_pll_config_t llm_pll_config = ECOS_RCU_PLL_CONFIG_DEFAULT;

/* 稳定读取 CLINT 64 位 mtime。 */
static uint64_t read_mtime(void)
{
    uint32_t high_before;
    uint32_t low;
    uint32_t high_after;

    do {
        high_before = *(volatile uint32_t *)MTIME_HIGH_ADDR;
        low = *(volatile uint32_t *)MTIME_LOW_ADDR;
        high_after = *(volatile uint32_t *)MTIME_HIGH_ADDR;
    } while (high_before != high_after);

    return ((uint64_t)high_after << 32) | low;
}

/* 固定指令数忙等（2 条指令/迭代），用 mtime 折算真实执行速度，
 * 验证 PLL 切换是否真实生效：after/before 应接近 10 倍提速。 */
static void busy_loop_iters(uint32_t iters)
{
    __asm__ volatile(
        "1: addi %0, %0, -1\n"
        "   bnez %0, 1b\n"
        : "+r"(iters)
        :
        : "memory");
}

static uint32_t measure_loop_ms(void)
{
    uint64_t start = read_mtime();

    busy_loop_iters(1000000u);
    return (uint32_t)((read_mtime() - start) / (MTIME_HZ / 1000u));
}

static void halt(void)
{
    for (;;)
        __asm__ volatile("nop");
}

static void console_puts(const char *text)
{
    size_t length = 0u;

    while (text[length] != '\0')
        ++length;
    (void)bsp_console_write(text, length);
}

static void emit_piece(const char *piece, void *ctx)
{
    (void)ctx;
    console_puts(piece);
}

/* 最小行编辑：可打印字符回显，Backspace 擦除，回车结束。 */
static size_t console_read_line(char *buffer, size_t capacity)
{
    size_t length = 0u;

    for (;;) {
        uint8_t byte;
        int result = bsp_console_try_read(&byte);

        ECOS_PANIC_ON_ERROR(LOG_TAG, result, "read console");
        if (result == 0)
            continue;

        if (byte == '\n') {
            console_puts("\n");
            return length;
        }
        if (byte == 0x7Fu || byte == '\b') {
            if (length != 0u) {
                --length;
                console_puts("\b \b");
            }
            continue;
        }
        if (byte >= 0x20u && byte <= 0x7Eu && length + 1u < capacity) {
            buffer[length++] = (char)byte;
            (void)bsp_console_write((const char *)&byte, 1u);
        }
    }
}

int main(void)
{
    int dim;
    int n_layers;
    int vocab_size;
    uint32_t cpu_hz = 0u;
    uint32_t loop_ms_pre_pll;
    uint32_t loop_ms_post_pll;
    char prompt[LLM_PROMPT_MAX_CHARS];

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");

    loop_ms_pre_pll = measure_loop_ms();

    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rcu_pll_apply(ECOS_RCU_DEFAULT, &llm_pll_config),
                        "apply PLL config");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rcu_pll_get_freq(ECOS_RCU_DEFAULT,
                                              ECOS_RCU_CLOCK_CPU, &cpu_hz),
                        "read CPU frequency");
    (void)ECOS_LOGI(LOG_TAG, "CPU clock: %u Hz", (unsigned)cpu_hz);

    loop_ms_post_pll = measure_loop_ms();
    (void)ECOS_LOGI(LOG_TAG, "loop 1M iters: %u ms pre-PLL, %u ms post-PLL",
                    (unsigned)loop_ms_pre_pll, (unsigned)loop_ms_post_pll);

    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        runq_init(llm_model_start, llm_tokenizer_start),
                        "initialize model");

    runq_model_info(&dim, &n_layers, &vocab_size);
    (void)ECOS_LOGI(LOG_TAG,
                    "model ready: dim=%d layers=%d vocab=%d, free SDRAM %u B",
                    dim, n_layers, vocab_size,
                    (unsigned)ecos_heap_free_bytes(ECOS_HEAP_ZONE_EXTERNAL));
    console_puts("TinyStories-260K story writer; type an opening line.\n");

    for (;;) {
        size_t prompt_len;
        int generated;
        uint64_t gen_start;
        uint32_t elapsed_ms;

        console_puts("you> ");
        prompt_len = console_read_line(prompt, sizeof(prompt));
        if (prompt_len == 0u)
            continue;
        prompt[prompt_len] = '\0';

        gen_start = read_mtime();
        generated = runq_generate(prompt, LLM_GEN_STEPS, LLM_TEMPERATURE,
                                  LLM_TOP_P, LLM_RNG_SEED, emit_piece, NULL);
        ECOS_PANIC_ON_ERROR(LOG_TAG, generated, "generate");

        elapsed_ms = (uint32_t)((read_mtime() - gen_start) /
                                (MTIME_HZ / 1000u));
        if (elapsed_ms == 0u)
            elapsed_ms = 1u;
        (void)ECOS_LOGI(LOG_TAG, "[%d tokens in %u ms, %u.%u tok/s]",
                        generated, (unsigned)elapsed_ms,
                        (unsigned)((uint32_t)generated * 1000u / elapsed_ms),
                        (unsigned)(((uint32_t)generated * 10000u /
                                    elapsed_ms) % 10u));

        {
            runq_profile_t prof;

            runq_profile_read(&prof);
            (void)ECOS_LOGI(LOG_TAG,
                            "[prof] matmul %u ms, attn %u ms, rope %u ms, "
                            "sample %u ms",
                            (unsigned)((uint32_t)prof.matmul_ticks / 5000u),
                            (unsigned)((uint32_t)prof.attention_ticks / 5000u),
                            (unsigned)((uint32_t)prof.rope_ticks / 5000u),
                            (unsigned)((uint32_t)prof.sample_ticks / 5000u));
        }
    }
}
