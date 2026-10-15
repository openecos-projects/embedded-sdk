/* Inference for Llama-2 Transformer model in pure C, int8 quantized forward pass.
 *
 * Vendored from llama2.c (https://github.com/karpathy/llama2.c, MIT License,
 * Copyright (c) 2023 Andrej Karpathy) and adapted for the ECOS bare-metal
 * T1-Pico target:
 * - checkpoint and tokenizer are read from memory pointers instead of
 *   files/mmap;
 * - allocations come from the ECOS heap component (internal SRAM pool for
 *   small hot buffers, external SDRAM pool for the KV cache and other
 *   large/cold buffers);
 * - group quantization tolerates a partial tail group: stories260K rows are
 *   64/32/172 wide, which upstream (GS divides row width) cannot run;
 * - tokenizer lookup is a linear scan over the 512-entry vocab instead of
 *   qsort/bsearch, and the "<0xHH>" byte fallback is parsed by hand;
 * - output is delivered through an emit callback; chat mode, CLI and timing
 *   are removed.
 */

#include "runq.h"

#include "ecos/error.h"
#include "ecos/heap.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/* 1 = 初始化时把权重体从 Flash XIP 整体搬迁到 SDRAM，用约 279 KB SDRAM
 * 换每个 token 的 QSPI 流式读带宽；0 = 保持 XIP 直读（用于 A/B 对比）。 */
#ifndef RUNQ_WEIGHTS_IN_SDRAM
#define RUNQ_WEIGHTS_IN_SDRAM 1
#endif

/* 1 = 热点函数经 .isram.text 段放入 ISRAM 运行（I-cache 的软件等价），
 * 消除热点循环的 Flash XIP 取指；目标不支持 ISRAM 执行时置 0 回退。 */
#ifndef RUNQ_CODE_IN_ISRAM
#define RUNQ_CODE_IN_ISRAM 1
#endif
#if RUNQ_CODE_IN_ISRAM
#define RUNQ_HOT __attribute__((section(".isram.text")))
#else
#define RUNQ_HOT
#endif

/* ------------------------------------------------------------------------- */
/* Quantization                                                               */

static int g_gs; /* group size for quantized weights, from the checkpoint */

/* ------------------------------------------------------------------------- */
/* Model                                                                      */

typedef struct {
    int dim;
    int hidden_dim;
    int n_layers;
    int n_heads;
    int n_kv_heads;
    int vocab_size;
    int seq_len;
} Config;

typedef struct {
    int8_t *q;   /* quantized values */
    float *s;    /* scaling factors, one per group (tail group may be short) */
} QuantizedTensor;

typedef struct {
    QuantizedTensor *q_tokens;     /* (vocab_size, dim) */
    float *token_embedding_table;  /* dequantized copy */

    float *rms_att_weight;         /* (layer, dim) */
    float *rms_ffn_weight;         /* (layer, dim) */
    QuantizedTensor *wq;           /* (layer, dim, dim) */
    QuantizedTensor *wk;           /* (layer, dim, kv_dim) */
    QuantizedTensor *wv;           /* (layer, dim, kv_dim) */
    QuantizedTensor *wo;           /* (layer, dim, dim) */
    QuantizedTensor *w1;           /* (layer, hidden_dim, dim) */
    QuantizedTensor *w2;           /* (layer, dim, hidden_dim) */
    QuantizedTensor *w3;           /* (layer, hidden_dim, dim) */
    float *rms_final_weight;       /* (dim,) */
    QuantizedTensor *wcls;
} TransformerWeights;

typedef struct {
    float *x;    /* (dim,) */
    float *xb;   /* (dim,) */
    float *xb2;  /* (dim,) */
    float *hb;   /* (hidden_dim,) */
    float *hb2;  /* (hidden_dim,) */
    QuantizedTensor xq;
    QuantizedTensor hq;
    float *q;    /* (dim,) */
    float *k;    /* (kv_dim,) */
    float *v;    /* (kv_dim,) */
    float *att;  /* (n_heads, seq_len) */
    float *logits; /* (vocab_size,) */
    float *key_cache;    /* (layer, seq_len, kv_dim) */
    float *value_cache;  /* (layer, seq_len, kv_dim) */
    float *rope_cis;     /* (seq_len, head_size) RoPE cos/sin 预计算表 */
} RunState;

typedef struct {
    Config config;
    TransformerWeights weights;
    RunState state;
} Transformer;

static Transformer g_tf;
static int g_ready;

/* 分阶段耗时统计：CLINT mtime 走时 5 MHz，与 Timer0 互不占用。
 * PC 仿真无 CLINT，恒返回 0，不影响输出对照。 */
static runq_profile_t g_profile;

#if defined(__riscv)
#define RUNQ_MTIME_LOW  (*(volatile uint32_t *)(uintptr_t)0x0200BFF8u)
#define RUNQ_MTIME_HIGH (*(volatile uint32_t *)(uintptr_t)0x0200BFFCu)

static uint64_t runq_mtime(void)
{
    uint32_t high_before;
    uint32_t low;
    uint32_t high_after;

    do {
        high_before = RUNQ_MTIME_HIGH;
        low = RUNQ_MTIME_LOW;
        high_after = RUNQ_MTIME_HIGH;
    } while (high_before != high_after);

    return ((uint64_t)high_after << 32) | low;
}
#else
static uint64_t runq_mtime(void)
{
    return 0u;
}
#endif

void runq_profile_read(runq_profile_t *out)
{
    /* 逐字段拷贝：裸机无 libc，结构体整体赋值会被编译成 memcpy。 */
    out->matmul_ticks = g_profile.matmul_ticks;
    out->rope_ticks = g_profile.rope_ticks;
    out->attention_ticks = g_profile.attention_ticks;
    out->sample_ticks = g_profile.sample_ticks;
}

/* 工具链 libm 的数学错误路径引用 newlib 的 __errno；本运行时无 errno
 * 语义，提供最小实现让链接通过（数学域错误对我们没有影响）。 */
static int g_runq_errno;
int *__errno(void)
{
    return &g_runq_errno;
}

static void *runq_alloc(size_t size, ecos_heap_zone_t zone)
{
    return ecos_heap_alloc(zone, size);
}

/* 小块热点缓冲进片上 SRAM，大块/冷数据进 SDRAM。 */
static void *runq_alloc_fast(size_t size)
{
    return runq_alloc(size, ECOS_HEAP_ZONE_INTERNAL);
}

static void *runq_alloc_bulk(size_t size)
{
    return runq_alloc(size, ECOS_HEAP_ZONE_EXTERNAL);
}

static void fcopy(float *dest, const float *src, int count)
{
    int i;

    for (i = 0; i < count; ++i)
        dest[i] = src[i];
}

static void fzero(float *dest, int count)
{
    int i;

    for (i = 0; i < count; ++i)
        dest[i] = 0.0f;
}

/* 权重体按 4 字节对齐整块拷贝（源为 .balign 4 的 Flash 镜像，长度为 4 的
 * 倍数，尾部字节循环仅兜底）。 */
static void runq_copy_block(void *dest, const void *src, size_t bytes)
{
    uint32_t *d = (uint32_t *)dest;
    const uint32_t *s = (const uint32_t *)src;
    size_t words = bytes / 4u;
    size_t i;

    for (i = 0; i < words; ++i)
        d[i] = s[i];
    if ((bytes & 3u) != 0u) {
        uint8_t *db = (uint8_t *)dest + words * 4u;
        const uint8_t *sb = (const uint8_t *)src + words * 4u;
        size_t j;

        for (j = 0; j < (bytes & 3u); ++j)
            db[j] = sb[j];
    }
}

/* 单个量化矩阵块的字节数：int8 载荷 + 每行 ceil(cols/GS) 个 fp32 scale。 */
static size_t quantized_block_bytes(int rows, int cols)
{
    int groups_per_row = (cols + g_gs - 1) / g_gs;

    return (size_t)rows * (size_t)cols +
           (size_t)rows * (size_t)groups_per_row * sizeof(float);
}

/* 权重体总字节数，与 quantize_q8.py 的写入布局一一对应：rms 权重 fp32，
 * 随后 token 表与逐层 wq/wk/wv/wo/w1/w2/w3 的量化块。 */
static size_t weight_body_bytes(const Config *p)
{
    int kv_dim = p->dim * p->n_kv_heads / p->n_heads;
    size_t per_layer;
    size_t total;

    total = (size_t)(2 * p->n_layers * p->dim + p->dim) * sizeof(float);
    total += quantized_block_bytes(p->vocab_size, p->dim);

    per_layer = quantized_block_bytes(p->dim, p->dim) +
                quantized_block_bytes(kv_dim, p->dim) +
                quantized_block_bytes(kv_dim, p->dim) +
                quantized_block_bytes(p->dim, p->dim) +
                quantized_block_bytes(p->hidden_dim, p->dim) +
                quantized_block_bytes(p->dim, p->hidden_dim) +
                quantized_block_bytes(p->hidden_dim, p->dim);
    return total + (size_t)p->n_layers * per_layer;
}

static int malloc_run_state(RunState *s, const Config *p)
{
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    int head_size = p->dim / p->n_heads;

    s->x = runq_alloc_fast((size_t)p->dim * sizeof(float));
    s->xb = runq_alloc_fast((size_t)p->dim * sizeof(float));
    s->xb2 = runq_alloc_fast((size_t)p->dim * sizeof(float));
    s->hb = runq_alloc_fast((size_t)p->hidden_dim * sizeof(float));
    s->hb2 = runq_alloc_fast((size_t)p->hidden_dim * sizeof(float));
    s->xq.q = runq_alloc_fast((size_t)p->dim);
    s->xq.s = runq_alloc_fast((size_t)p->dim * sizeof(float));
    s->hq.q = runq_alloc_fast((size_t)p->hidden_dim);
    s->hq.s = runq_alloc_fast((size_t)p->hidden_dim * sizeof(float));
    s->q = runq_alloc_fast((size_t)p->dim * sizeof(float));
    s->k = runq_alloc_fast((size_t)kv_dim * sizeof(float));
    s->v = runq_alloc_fast((size_t)kv_dim * sizeof(float));
    s->logits = runq_alloc_fast((size_t)p->vocab_size * sizeof(float));
    s->att = runq_alloc_bulk((size_t)p->n_heads * p->seq_len * sizeof(float));
    s->key_cache = runq_alloc_bulk(
        (size_t)p->n_layers * p->seq_len * kv_dim * sizeof(float));
    s->value_cache = runq_alloc_bulk(
        (size_t)p->n_layers * p->seq_len * kv_dim * sizeof(float));
    s->rope_cis = runq_alloc_bulk(
        (size_t)p->seq_len * (size_t)head_size * 2u * sizeof(float));

    if (!s->x || !s->xb || !s->xb2 || !s->hb || !s->hb2 || !s->xq.q ||
        !s->xq.s || !s->hq.q || !s->hq.s || !s->q || !s->k || !s->v ||
        !s->logits || !s->att || !s->key_cache || !s->value_cache ||
        !s->rope_cis)
        return ECOS_ERR_NO_MEMORY;
    return ECOS_OK;
}

/* RoPE 预计算表：freq 只取决于 head_dim，val = pos x freq，全部组合在
 * 初始化时一次算清，前向纯查表。数值与逐点调用 powf/cosf/sinf 完全一致
 * （小整数转 float 精确，乘除次序相同），贪心输出不受影响。 */
static void precompute_rope_cis(RunState *s, const Config *p)
{
    int head_size = p->dim / p->n_heads;
    int pos;
    int hd;

    for (hd = 0; hd < head_size; ++hd) {
        float freq = 1.0f / powf(10000.0f, (float)hd / (float)head_size);

        for (pos = 0; pos < p->seq_len; ++pos) {
            float val = (float)pos * freq;
            float *entry = s->rope_cis +
                           ((size_t)pos * (size_t)head_size + (size_t)hd) * 2u;

            entry[0] = cosf(val);
            entry[1] = sinf(val);
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Quantization helpers                                                       */

/* init n QuantizedTensor views (rows x cols elements each) into the
 * checkpoint image at *ptr: int8 payload first, then fp32 scales
 * (rows * ceil(cols/GS) entries — 每组沿行内计数，不足一组有尾组）。 */
static QuantizedTensor *init_quantized_tensors(const uint8_t **ptr, int n,
                                               int rows, int cols)
{
    int groups_per_row = (cols + g_gs - 1) / g_gs;
    const uint8_t *p = *ptr;
    QuantizedTensor *res = runq_alloc_fast((size_t)n * sizeof(QuantizedTensor));
    int i;

    if (res == NULL)
        return NULL;
    for (i = 0; i < n; ++i) {
        res[i].q = (int8_t *)p;
        p += (size_t)rows * cols;
        res[i].s = (float *)p;
        p += (size_t)rows * groups_per_row * sizeof(float);
    }
    *ptr = p;
    return res;
}

RUNQ_HOT static void dequantize(QuantizedTensor *qx, float *x, int n)
{
    int i;

    for (i = 0; i < n; ++i)
        x[i] = qx->q[i] * qx->s[i / g_gs];
}

RUNQ_HOT static void quantize(QuantizedTensor *qx, float *x, int n)
{
    int group;

    for (group = 0; group < n; group += g_gs) {
        int gs = g_gs < n - group ? g_gs : n - group;
        float wmax = 0.0f;
        float scale;
        int i;

        for (i = 0; i < gs; ++i) {
            float val = fabsf(x[group + i]);

            if (val > wmax)
                wmax = val;
        }

        if (wmax == 0.0f) {
            /* 全零分组：scale 记 0，量化值记 0，反量化/矩阵乘自然得零。 */
            qx->s[group / g_gs] = 0.0f;
            for (i = 0; i < gs; ++i)
                qx->q[group + i] = 0;
            continue;
        }

        scale = wmax / 127.0f;
        qx->s[group / g_gs] = scale;
        for (i = 0; i < gs; ++i) {
            float quant_value = x[group + i] / scale;
            int quantized = (int)roundf(quant_value);

            if (quantized > 127)
                quantized = 127;
            if (quantized < -127)
                quantized = -127;
            qx->q[group + i] = (int8_t)quantized;
        }
    }
}

static int map_weights(TransformerWeights *w, const Config *p,
                       const uint8_t *ptr, int shared_classifier)
{
    int kv_dim = p->dim * p->n_kv_heads / p->n_heads;
    const float *fptr = (const float *)ptr;
    size_t table_size = (size_t)p->vocab_size * p->dim * sizeof(float);

    w->rms_att_weight = (float *)fptr;
    fptr += p->n_layers * p->dim;
    w->rms_ffn_weight = (float *)fptr;
    fptr += p->n_layers * p->dim;
    w->rms_final_weight = (float *)fptr;
    fptr += p->dim;

    /* 行/列语义对齐 matmul：W(rows=d 输出维, cols=n 输入维)。 */
    ptr = (const uint8_t *)fptr;
    w->q_tokens = init_quantized_tensors(&ptr, 1, p->vocab_size, p->dim);
    w->wq = init_quantized_tensors(&ptr, p->n_layers, p->dim, p->dim);
    w->wk = init_quantized_tensors(&ptr, p->n_layers, kv_dim, p->dim);
    w->wv = init_quantized_tensors(&ptr, p->n_layers, kv_dim, p->dim);
    w->wo = init_quantized_tensors(&ptr, p->n_layers, p->dim, p->dim);
    w->w1 = init_quantized_tensors(&ptr, p->n_layers, p->hidden_dim, p->dim);
    w->w2 = init_quantized_tensors(&ptr, p->n_layers, p->dim, p->hidden_dim);
    w->w3 = init_quantized_tensors(&ptr, p->n_layers, p->hidden_dim, p->dim);
    if (w->q_tokens == NULL || w->wq == NULL || w->wk == NULL ||
        w->wv == NULL || w->wo == NULL || w->w1 == NULL || w->w2 == NULL ||
        w->w3 == NULL)
        return ECOS_ERR_NO_MEMORY;

    w->wcls = shared_classifier
                  ? w->q_tokens
                  : init_quantized_tensors(&ptr, 1, p->vocab_size, p->dim);
    if (w->wcls == NULL)
        return ECOS_ERR_NO_MEMORY;

    /* 嵌入表反量化为 fp32，只在初始化时做一次。 */
    w->token_embedding_table = runq_alloc_bulk(table_size);
    if (w->token_embedding_table == NULL)
        return ECOS_ERR_NO_MEMORY;
    dequantize(w->q_tokens, w->token_embedding_table, p->vocab_size * p->dim);
    return ECOS_OK;
}

/* ------------------------------------------------------------------------- */
/* Transformer forward                                                        */

RUNQ_HOT static void rmsnorm(float *o, const float *x, const float *weight, int size)
{
    float ss = 0.0f;
    int j;

    for (j = 0; j < size; ++j)
        ss += x[j] * x[j];
    ss /= size;
    ss += 1e-5f;
    ss = 1.0f / sqrtf(ss);
    for (j = 0; j < size; ++j)
        o[j] = weight[j] * (ss * x[j]);
}

/* 快速 expf：exp(x) = 2^(x*log2e)，整数部分直接构造 float 指数位，
 * 小数部分用 4 次多项式逼近。输入钳制到 [-87, 88]，相对误差约 1e-4 量级，
 * 远小于 int8 量化噪声，供 softmax/sigmoid 使用。 */
RUNQ_HOT static float fast_expf(float x)
{
    union {
        uint32_t u;
        float f;
    } scale;
    float y;
    float f;
    float p;
    int n;

    if (x > 88.0f)
        x = 88.0f;
    if (x < -87.0f)
        x = -87.0f;

    y = x * 1.4426950408889634f; /* log2(e) */
    n = (int)y;
    if ((float)n > y)
        n -= 1; /* C 语法向零取整，修正为向下取整 */
    f = y - (float)n;

    /* 2^f，f ∈ [0,1)，minimax 四次多项式 */
    p = 1.0000026f +
        f * (0.6930048f +
             f * (0.2413736f + f * (0.0520352f + f * 0.0134042f)));

    scale.u = (uint32_t)(n + 127) << 23;
    return p * scale.f;
}

RUNQ_HOT static void softmax(float *x, int size)
{
    float max_val = x[0];
    float sum = 0.0f;
    int i;

    for (i = 1; i < size; ++i) {
        if (x[i] > max_val)
            max_val = x[i];
    }
    for (i = 0; i < size; ++i) {
        x[i] = fast_expf(x[i] - max_val);
        sum += x[i];
    }
    /* 一次除法换 n 次乘法：软浮点除法比乘法贵数倍。 */
    {
        float inv_sum = 1.0f / sum;
        for (i = 0; i < size; ++i)
            x[i] *= inv_sum;
    }
}

/* xout(d,) = W(d,n) @ x(n,)；两个输入均为分组量化。最耗时的函数。 */
RUNQ_HOT static void matmul(float *xout, const QuantizedTensor *x,
                   const QuantizedTensor *w, int n, int d)
{
    uint64_t t0 = runq_mtime();
    int groups_per_row = (n + g_gs - 1) / g_gs;
    int i;

    for (i = 0; i < d; ++i) {
        float val = 0.0f;
        int32_t ival = 0;
        int in = i * n;
        int j;

        for (j = 0; j < n; j += g_gs) {
            int gs = g_gs < n - j ? g_gs : n - j;
            int k;

            /* 逐字节加载：rv32imc 无 sext.b，按字打包的拆包指令开销
             * 超过访存节省（实测为负优化，已回退）。 */
            for (k = 0; k < gs; ++k)
                ival += ((int32_t)x->q[j + k]) * ((int32_t)w->q[in + j + k]);
            /* 权重的 scale 按 行 × ceil(n/GS) 布局，不能用 (in+j)/GS。 */
            val += (float)ival * w->s[i * groups_per_row + j / g_gs] *
                   x->s[j / g_gs];
            ival = 0;
        }
        xout[i] = val;
    }
    g_profile.matmul_ticks += runq_mtime() - t0;
}

static float *forward(Transformer *transformer, int token, int pos)
{
    Config *p = &transformer->config;
    TransformerWeights *w = &transformer->weights;
    RunState *s = &transformer->state;
    float *x = s->x;
    int dim = p->dim;
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    int kv_mul = p->n_heads / p->n_kv_heads;
    int hidden_dim = p->hidden_dim;
    int head_size = dim / p->n_heads;
    /* 每次前向只算一次：注意力打分原本在逐 head 逐 pos 重复调用 sqrtf。 */
    float inv_sqrt_head_size = 1.0f / sqrtf((float)head_size);
    int l;

    fcopy(x, w->token_embedding_table + token * dim, dim);

    for (l = 0; l < p->n_layers; ++l) {
        int loff = l * p->seq_len * kv_dim;
        int h;
        int i;

        rmsnorm(s->xb, x, w->rms_att_weight + l * dim, dim);

        quantize(&s->xq, s->xb, dim);
        matmul(s->q, &s->xq, w->wq + l, dim, dim);
        matmul(s->k, &s->xq, w->wk + l, dim, kv_dim);
        matmul(s->v, &s->xq, w->wv + l, dim, kv_dim);

        /* RoPE 相对位置编码：cos/sin 查初始化时的预计算表 */
        {
            uint64_t t0 = runq_mtime();

            for (i = 0; i < dim; i += 2) {
                int head_dim = i % head_size;
                const float *cis = s->rope_cis +
                    ((size_t)pos * (size_t)head_size + (size_t)head_dim) * 2u;
                float fcr = cis[0];
                float fci = cis[1];
                int rotn = i < kv_dim ? 2 : 1;
                int v;

                for (v = 0; v < rotn; ++v) {
                    float *vec = v == 0 ? s->q : s->k;
                    float v0 = vec[i];
                    float v1 = vec[i + 1];

                    vec[i] = v0 * fcr - v1 * fci;
                    vec[i + 1] = v0 * fci + v1 * fcr;
                }
            }
            g_profile.rope_ticks += runq_mtime() - t0;
        }

        fcopy(s->key_cache + loff + pos * kv_dim, s->k, kv_dim);
        fcopy(s->value_cache + loff + pos * kv_dim, s->v, kv_dim);

        {
            uint64_t t0 = runq_mtime();

            for (h = 0; h < p->n_heads; ++h) {
                float *q = s->q + h * head_size;
                float *att = s->att + h * p->seq_len;
                float *xb = s->xb + h * head_size;
                int t;

                for (t = 0; t <= pos; ++t) {
                    float *k = s->key_cache + loff + t * kv_dim +
                               (h / kv_mul) * head_size;
                    float score = 0.0f;

                    for (i = 0; i < head_size; ++i)
                        score += q[i] * k[i];
                    score *= inv_sqrt_head_size;
                    att[t] = score;
                }

                softmax(att, pos + 1);

                fzero(xb, head_size);
                for (t = 0; t <= pos; ++t) {
                    float *v = s->value_cache + loff + t * kv_dim +
                               (h / kv_mul) * head_size;
                    float a = att[t];

                    for (i = 0; i < head_size; ++i)
                        xb[i] += a * v[i];
                }
            }
            g_profile.attention_ticks += runq_mtime() - t0;
        }

        quantize(&s->xq, s->xb, dim);
        matmul(s->xb2, &s->xq, w->wo + l, dim, dim);

        for (i = 0; i < dim; ++i)
            x[i] += s->xb2[i];

        rmsnorm(s->xb, x, w->rms_ffn_weight + l * dim, dim);

        quantize(&s->xq, s->xb, dim);
        matmul(s->hb, &s->xq, w->w1 + l, dim, hidden_dim);
        matmul(s->hb2, &s->xq, w->w3 + l, dim, hidden_dim);

        /* SwiGLU：silu(x)=x*σ(x) */
        for (i = 0; i < hidden_dim; ++i) {
            float val = s->hb[i];

            val *= 1.0f / (1.0f + fast_expf(-val));
            val *= s->hb2[i];
            s->hb[i] = val;
        }

        quantize(&s->hq, s->hb, hidden_dim);
        matmul(s->xb, &s->hq, w->w2 + l, hidden_dim, dim);

        for (i = 0; i < dim; ++i)
            x[i] += s->xb[i];
    }

    rmsnorm(x, x, w->rms_final_weight, dim);

    quantize(&s->xq, x, dim);
    matmul(s->logits, &s->xq, w->wcls, dim, p->vocab_size);
    return s->logits;
}

/* ------------------------------------------------------------------------- */
/* BPE tokenizer                                                              */

typedef struct {
    char **vocab;
    float *vocab_scores;
    char *vocab_blob;
    char *merge_buffer;
    int vocab_size;
    unsigned int max_token_length;
    unsigned char byte_pieces[512];
} Tokenizer;

static Tokenizer g_tok;

static int build_tokenizer(Tokenizer *t, const uint8_t *data, int vocab_size)
{
    const uint8_t *cursor = data;
    uint32_t max_token_length;
    char *blob;
    size_t blob_used = 0u;
    int i;

    t->vocab_size = vocab_size;
    for (i = 0; i < 256; ++i) {
        t->byte_pieces[i * 2] = (unsigned char)i;
        t->byte_pieces[i * 2 + 1] = '\0';
    }

    t->vocab = runq_alloc_fast((size_t)vocab_size * sizeof(char *));
    t->vocab_scores = runq_alloc_bulk((size_t)vocab_size * sizeof(float));
    if (t->vocab == NULL || t->vocab_scores == NULL)
        return ECOS_ERR_NO_MEMORY;

    max_token_length = (uint32_t)cursor[0] | ((uint32_t)cursor[1] << 8) |
                       ((uint32_t)cursor[2] << 16) | ((uint32_t)cursor[3] << 24);
    cursor += 4;
    t->max_token_length = max_token_length;

    /* 词表字符串连续打包进一块 SDRAM 缓冲，逐条 NUL 结尾。 */
    blob = runq_alloc_bulk((size_t)vocab_size * (max_token_length + 1u));
    if (blob == NULL)
        return ECOS_ERR_NO_MEMORY;
    t->vocab_blob = blob;

    for (i = 0; i < vocab_size; ++i) {
        uint32_t len;
        uint32_t c;

        /* score 是 IEEE-754 位形，按位组装后 reinterpret */
        {
            union {
                uint32_t u;
                float f;
            } score_bits;

            score_bits.u = (uint32_t)cursor[0] | ((uint32_t)cursor[1] << 8) |
                           ((uint32_t)cursor[2] << 16) |
                           ((uint32_t)cursor[3] << 24);
            t->vocab_scores[i] = score_bits.f;
        }
        cursor += 4;
        len = (uint32_t)cursor[0] | ((uint32_t)cursor[1] << 8) |
              ((uint32_t)cursor[2] << 16) | ((uint32_t)cursor[3] << 24);
        cursor += 4;
        if (len > max_token_length)
            return ECOS_ERR_INVALID_ARGUMENT;

        t->vocab[i] = blob + blob_used;
        for (c = 0u; c < len; ++c)
            blob[blob_used++] = (char)*cursor++;
        blob[blob_used++] = '\0';
    }

    t->merge_buffer = runq_alloc_fast(max_token_length * 2u + 3u);
    if (t->merge_buffer == NULL)
        return ECOS_ERR_NO_MEMORY;
    return ECOS_OK;
}

static char *decode(Tokenizer *t, int prev_token, int token)
{
    char *piece = t->vocab[token];

    /* BOS 之后 sentencepiece 会去掉前导空格 */
    if (prev_token == 1 && piece[0] == ' ')
        ++piece;

    /* 字节回退片段形如 "<0x0A>"，手工解析为对应字节。 */
    if (piece[0] == '<' && piece[1] == '0' && piece[2] == 'x' &&
        piece[3] != '\0' && piece[4] != '\0' && piece[5] == '>' &&
        piece[6] == '\0') {
        int hi = piece[3];
        int lo = piece[4];
        unsigned int byte_val;

        hi = hi >= '0' && hi <= '9'   ? hi - '0'
             : hi >= 'A' && hi <= 'F' ? hi - 'A' + 10
             : hi >= 'a' && hi <= 'f' ? hi - 'a' + 10
                                      : -1;
        lo = lo >= '0' && lo <= '9'   ? lo - '0'
             : lo >= 'A' && lo <= 'F' ? lo - 'A' + 10
             : lo >= 'a' && lo <= 'f' ? lo - 'a' + 10
                                      : -1;
        if (hi >= 0 && lo >= 0) {
            byte_val = (unsigned int)(hi * 16 + lo);
            return (char *)t->byte_pieces + byte_val * 2;
        }
    }
    return piece;
}

static int runq_is_print(unsigned char c)
{
    return c >= 0x20u && c <= 0x7Eu;
}

static int runq_is_space(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
           c == '\f';
}

/* 词表只有 512 项，线性查找足够；按 str 精确匹配返回 id 或 -1。 */
static int str_lookup(Tokenizer *t, const char *str)
{
    int i;

    for (i = 0; i < t->vocab_size; ++i) {
        const char *candidate = t->vocab[i];
        int j = 0;

        while (candidate[j] != '\0' && candidate[j] == str[j])
            ++j;
        if (candidate[j] == '\0' && str[j] == '\0')
            return i;
    }
    return -1;
}

static void merge_concat(char *out, const char *a, const char *b)
{
    while (*a != '\0')
        *out++ = *a++;
    while (*b != '\0')
        *out++ = *b++;
    *out = '\0';
}

static int encode(Tokenizer *t, const char *text, int8_t bos, int *tokens,
                  int max_tokens)
{
    int n_tokens = 0;
    char *str_buffer = t->merge_buffer;
    size_t str_len = 0u;
    const char *c;

    if (bos != 0)
        tokens[n_tokens++] = 1;

    /* 与上游一致：非空文本前置一个 dummy 空格 token */
    if (text[0] != '\0') {
        int dummy_prefix = str_lookup(t, " ");

        if (dummy_prefix >= 0 && n_tokens < max_tokens)
            tokens[n_tokens++] = dummy_prefix;
    }

    for (c = text; *c != '\0'; ++c) {
        int id;

        if ((*c & 0xC0) != 0x80)
            str_len = 0u; /* 非 UTF-8 延续字节，开始新码点 */

        str_buffer[str_len++] = *c;
        str_buffer[str_len] = '\0';

        if ((c[1] & 0xC0) == 0x80 && str_len < 4u)
            continue; /* 继续收集延续字节 */

        id = str_lookup(t, str_buffer);
        if (id >= 0) {
            if (n_tokens >= max_tokens)
                break;
            tokens[n_tokens++] = id;
        } else {
            /* byte_fallback：前 3 项是 <unk>/<s>/</s>，字节从 3 开始 */
            size_t b;

            for (b = 0u; b < str_len; ++b) {
                if (n_tokens >= max_tokens)
                    break;
                tokens[n_tokens++] = (unsigned char)str_buffer[b] + 3;
            }
        }
        str_len = 0u;
    }

    /* 按 score 反复合并最优相邻对 */
    for (;;) {
        float best_score = -1e10f;
        int best_id = -1;
        int best_idx = -1;
        int i;

        for (i = 0; i < n_tokens - 1; ++i) {
            int id;

            merge_concat(str_buffer, t->vocab[tokens[i]],
                         t->vocab[tokens[i + 1]]);
            id = str_lookup(t, str_buffer);
            if (id >= 0 && t->vocab_scores[id] > best_score) {
                best_score = t->vocab_scores[id];
                best_id = id;
                best_idx = i;
            }
        }
        if (best_idx == -1)
            break;

        tokens[best_idx] = best_id;
        for (i = best_idx + 1; i < n_tokens - 1; ++i)
            tokens[i] = tokens[i + 1];
        --n_tokens;
    }
    return n_tokens;
}

/* ------------------------------------------------------------------------- */
/* Sampler                                                                    */

typedef struct {
    float prob;
    int index;
} ProbIndex;

typedef struct {
    int vocab_size;
    ProbIndex *probindex;
    float temperature;
    float topp;
    unsigned long long rng_state;
} Sampler;

static Sampler g_sampler;

static int sample_argmax(const float *probabilities, int n)
{
    int max_i = 0;
    float max_p = probabilities[0];
    int i;

    for (i = 1; i < n; ++i) {
        if (probabilities[i] > max_p) {
            max_i = i;
            max_p = probabilities[i];
        }
    }
    return max_i;
}

static int sample_mult(const float *probabilities, int n, float coin)
{
    float cdf = 0.0f;
    int i;

    for (i = 0; i < n; ++i) {
        cdf += probabilities[i];
        if (coin < cdf)
            return i;
    }
    return n - 1;
}

static int sample_topp(float *probabilities, int n, float topp,
                       ProbIndex *probindex, float coin)
{
    int n0 = 0;
    float cutoff = (1.0f - topp) / (n - 1);
    float cumulative_prob = 0.0f;
    float r;
    float cdf = 0.0f;
    int picked = 0;
    int last_idx;
    int i;

    for (i = 0; i < n; ++i) {
        if (probabilities[i] >= cutoff) {
            probindex[n0].index = i;
            probindex[n0].prob = probabilities[i];
            ++n0;
        }
    }

    /* top-p 只需要降序前缀：每轮把剩余最大值交换进前缀，累积概率超过
     * topp 即停，避免对全部候选做 O(n^2) 插入排序。概率完全相等时与原先
     * 稳定全排的相对先后可能不同，但不改变各 token 被选中的概率。 */
    while (picked < n0) {
        if (picked < n0 - 1) {
            int best = picked;
            int j;
            ProbIndex tmp;

            for (j = picked + 1; j < n0; ++j) {
                if (probindex[j].prob > probindex[best].prob)
                    best = j;
            }
            tmp = probindex[picked];
            probindex[picked] = probindex[best];
            probindex[best] = tmp;
        }
        cumulative_prob += probindex[picked].prob;
        if (cumulative_prob > topp)
            break;
        ++picked;
    }
    last_idx = picked < n0 ? picked : n0 - 1;

    r = coin * cumulative_prob;
    for (i = 0; i <= last_idx; ++i) {
        cdf += probindex[i].prob;
        if (r < cdf)
            return probindex[i].index;
    }
    return probindex[last_idx].index;
}

static unsigned int random_u32(unsigned long long *state)
{
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    return (*state * 0x2545F4914F6CDD1Dull) >> 32;
}

static float random_f32(unsigned long long *state)
{
    return (random_u32(state) >> 8) / 16777216.0f;
}

static int sample(Sampler *sampler, float *logits)
{
    int q;

    if (sampler->temperature == 0.0f)
        return sample_argmax(logits, sampler->vocab_size);

    for (q = 0; q < sampler->vocab_size; ++q)
        logits[q] /= sampler->temperature;
    softmax(logits, sampler->vocab_size);

    {
        float coin = random_f32(&sampler->rng_state);

        if (sampler->topp <= 0.0f || sampler->topp >= 1.0f)
            return sample_mult(logits, sampler->vocab_size, coin);
        return sample_topp(logits, sampler->vocab_size, sampler->topp,
                           sampler->probindex, coin);
    }
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */

void runq_model_info(int *dim, int *n_layers, int *vocab_size)
{
    if (dim != NULL)
        *dim = g_tf.config.dim;
    if (n_layers != NULL)
        *n_layers = g_tf.config.n_layers;
    if (vocab_size != NULL)
        *vocab_size = g_tf.config.vocab_size;
}

static uint32_t runq_read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

int runq_init(const void *model_bin, const void *tokenizer_bin)
{
    const uint8_t *header = (const uint8_t *)model_bin;
    Config *config = &g_tf.config;
    uint8_t shared_classifier;
    int32_t group_size;
    int result;

    if (model_bin == NULL || tokenizer_bin == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* 头字段逐字节组装，避免非对齐访问。 */
    if (runq_read_u32(header) != 0x616B3432u ||
        runq_read_u32(header + 4) != 2u)
        return ECOS_ERR_INVALID_ARGUMENT;
    config->dim = (int32_t)runq_read_u32(header + 8);
    config->hidden_dim = (int32_t)runq_read_u32(header + 12);
    config->n_layers = (int32_t)runq_read_u32(header + 16);
    config->n_heads = (int32_t)runq_read_u32(header + 20);
    config->n_kv_heads = (int32_t)runq_read_u32(header + 24);
    config->vocab_size = (int32_t)runq_read_u32(header + 28);
    config->seq_len = (int32_t)runq_read_u32(header + 32);
    shared_classifier = header[36];
    group_size = (int32_t)runq_read_u32(header + 37);
    if (config->dim <= 0 || config->hidden_dim <= 0 || config->n_layers <= 0 ||
        config->n_heads <= 0 || config->n_kv_heads <= 0 ||
        config->vocab_size <= 0 || config->seq_len <= 0 || group_size <= 0)
        return ECOS_ERR_INVALID_ARGUMENT;
    g_gs = group_size;

    {
        const uint8_t *weights_src = header + 256;

#if RUNQ_WEIGHTS_IN_SDRAM
        /* 权重体整体搬迁到 SDRAM，后续前向不再 XIP 直读 Flash。 */
        size_t body_bytes = weight_body_bytes(config);
        uint8_t *weights_ram = runq_alloc_bulk(body_bytes);

        if (weights_ram == NULL)
            return ECOS_ERR_NO_MEMORY;
        runq_copy_block(weights_ram, weights_src, body_bytes);
        weights_src = weights_ram;
#endif

        result = map_weights(&g_tf.weights, config, weights_src,
                             shared_classifier);
    }
    if (result != ECOS_OK)
        return result;

    result = malloc_run_state(&g_tf.state, config);
    if (result != ECOS_OK)
        return result;

    precompute_rope_cis(&g_tf.state, config);

    result = build_tokenizer(&g_tok, (const uint8_t *)tokenizer_bin,
                             config->vocab_size);
    if (result != ECOS_OK)
        return result;

    g_sampler.probindex =
        runq_alloc_fast((size_t)config->vocab_size * sizeof(ProbIndex));
    if (g_sampler.probindex == NULL)
        return ECOS_ERR_NO_MEMORY;
    g_sampler.vocab_size = config->vocab_size;

    g_ready = 1;
    return ECOS_OK;
}

int runq_generate(const char *prompt, int steps, float temperature,
                  float topp, unsigned long long seed,
                  runq_emit_fn emit, void *ctx)
{
    int max_tokens;
    int *prompt_tokens;
    int num_prompt_tokens;
    int next;
    int token;
    int pos = 0;
    int generated = 0;

    if (g_ready == 0)
        return ECOS_ERR_NOT_INITIALIZED;
    if (prompt == NULL || prompt[0] == '\0' || emit == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (steps <= 0 || steps > g_tf.config.seq_len)
        steps = g_tf.config.seq_len;

    {
        size_t prompt_len = 0u;

        while (prompt[prompt_len] != '\0')
            ++prompt_len;
        max_tokens = (int)prompt_len + 4;
    }
    prompt_tokens =
        runq_alloc_fast((size_t)max_tokens * sizeof(int));
    if (prompt_tokens == NULL)
        return ECOS_ERR_NO_MEMORY;

    num_prompt_tokens =
        encode(&g_tok, prompt, 1, prompt_tokens, max_tokens);
    if (num_prompt_tokens < 1) {
        ecos_heap_free(prompt_tokens);
        return ECOS_ERR_INVALID_ARGUMENT;
    }

    g_sampler.temperature = temperature;
    g_sampler.topp = topp;
    g_sampler.rng_state = seed != 0ull ? seed : 1ull;

    g_profile.matmul_ticks = 0u;
    g_profile.rope_ticks = 0u;
    g_profile.attention_ticks = 0u;
    g_profile.sample_ticks = 0u;

    token = prompt_tokens[0];
    while (pos < steps) {
        float *logits = forward(&g_tf, token, pos);
        char *piece;

        if (pos < num_prompt_tokens - 1) {
            next = prompt_tokens[pos + 1];
        } else {
            uint64_t t0 = runq_mtime();

            next = sample(&g_sampler, logits);
            g_profile.sample_ticks += runq_mtime() - t0;
        }
        ++pos;

        /* BOS(=1) 作为序列分隔符时停止 */
        if (next == 1)
            break;

        piece = decode(&g_tok, token, next);

        /* 只输出可打印字符与空白，跳过控制字节片段。 */
        if (piece != NULL && piece[0] != '\0' &&
            (piece[1] == '\0'
                 ? (runq_is_print((unsigned char)piece[0]) ||
                    runq_is_space((unsigned char)piece[0]))
                 : 1)) {
            emit(piece, ctx);
        }
        token = next;
        ++generated;
    }

    ecos_heap_free(prompt_tokens);
    return generated;
}
