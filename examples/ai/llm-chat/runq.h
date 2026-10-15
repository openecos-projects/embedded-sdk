#ifndef RUNQ_H
#define RUNQ_H

#include <stdint.h>

/* 向控制台（或其他输出）推送一个生成片段的回调。 */
typedef void (*runq_emit_fn)(const char *text, void *ctx);

/* 从内存中的量化模型与词表初始化推理引擎；0 表示成功，负值为 ECOS_ERR_*。 */
int runq_init(const void *model_bin, const void *tokenizer_bin);

/* 以 prompt 为开头生成最多 steps 个 token，每个片段经 emit 流式输出；
 * 返回实际生成的 token 数，负值为 ECOS_ERR_*。 */
int runq_generate(const char *prompt, int steps, float temperature,
                  float topp, unsigned long long seed,
                  runq_emit_fn emit, void *ctx);

/* 读取模型结构参数，用于 banner 展示。 */
void runq_model_info(int *dim, int *n_layers, int *vocab_size);

/* 上一轮 runq_generate 的分阶段耗时（CLINT mtime ticks，5 MHz；
 * PC 仿真恒为 0）。 */
typedef struct {
    uint64_t matmul_ticks;
    uint64_t rope_ticks;
    uint64_t attention_ticks;
    uint64_t sample_ticks;
} runq_profile_t;

void runq_profile_read(runq_profile_t *out);

#endif
