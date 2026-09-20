#!/usr/bin/env python3
"""把 llama2.c 的 fp32 模型（run.c 格式，如 stories260K.bin）量化为
runq.c v2 的 int8 分组格式（Q8_0 风格，按行分组 absmax 缩放）。

用法：
    python3 quantize_q8.py stories260K.bin stories260k_q8.bin [group_size]

仅用标准库。group_size 默认 64（llama2.c 官方量化约定）。
"""

import struct
import sys

RUN_Q_MAGIC = 0x616B3432  # "ak42"
RUN_Q_VERSION = 2
RUN_Q_HEADER_SIZE = 256
Q_MAX = 127.0


def read_f32(data, offset, count):
    return list(struct.unpack_from("<%df" % count, data, offset))


def quantize_rows(out, values, rows, cols, group_size):
    """逐行按 group_size 分组量化：先写全部 int8，再写全部 fp32 scale。
    末组不足 group_size 时按实际长度量化（消费端按 ceil 分组读取）。"""
    scales = bytearray()
    quants = bytearray()
    for row in range(rows):
        base = row * cols
        for group_start in range(0, cols, group_size):
            group_end = base + min(group_start + group_size, cols)
            group = values[base + group_start: group_end]
            amax = max(abs(v) for v in group)
            scale = max(amax, 1e-8) / Q_MAX
            scales += struct.pack("<f", scale)
            for v in group:
                quants += struct.pack("b", max(-127, min(127, round(v / scale))))
    out += quants
    out += scales


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    src_path = sys.argv[1]
    dst_path = sys.argv[2]
    group_size = int(sys.argv[3]) if len(sys.argv) > 3 else 64

    with open(src_path, "rb") as src:
        data = src.read()

    # run.c fp32 头：7 个 int32 配置；vocab_size > 0 表示共享分类器
    dim, hidden_dim, n_layers, n_heads, n_kv_heads, vocab_size, seq_len = \
        struct.unpack_from("<7i", data, 0)
    if vocab_size <= 0:
        raise SystemExit("本脚本假定共享分类器（vocab_size > 0）")
    head_size = dim // n_heads
    kv_dim = dim * n_kv_heads // n_heads

    offset = 7 * 4

    def take(count):
        nonlocal offset
        values = read_f32(data, offset, count)
        offset += count * 4
        return values

    # run.c fp32 布局：embed → rms_att → wq/wk/wv/wo → rms_ffn → w1/w2/w3 →
    # rms_final →（遗留 rope 缓存，跳过）
    token_embedding = take(vocab_size * dim)
    rms_att = take(n_layers * dim)
    wq = take(n_layers * dim * dim)
    wk = take(n_layers * dim * kv_dim)
    wv = take(n_layers * dim * kv_dim)
    wo = take(n_layers * dim * dim)
    rms_ffn = take(n_layers * dim)
    w1 = take(n_layers * hidden_dim * dim)
    w2 = take(n_layers * dim * hidden_dim)
    w3 = take(n_layers * hidden_dim * dim)
    rms_final = take(dim)
    # 跳过 run.c 遗留的 rope 缓存区（不使用）
    offset += 2 * (seq_len * head_size // 2) * 4

    body = bytearray()
    # 先放保持 fp32 的 rmsnorm 权重
    for values in (rms_att, rms_ffn, rms_final):
        body += struct.pack("<%df" % len(values), *values)
    # 再放量化权重：token 表，然后逐层 wq/wk/wv/wo 与 w1/w2/w3
    quantize_rows(body, token_embedding, vocab_size, dim, group_size)
    # 逐层矩阵在文件中是按层交错的（每层：int8 块 + scale 块），
    # 与 runq.c 的 init_quantized_tensors 读取方式一一对应。
    # 行/列语义对齐 matmul：W(d rows, n cols)，即 cols 为输入维度。
    for matrix, rows_per_layer, cols in (
            (wq, dim, dim), (wk, kv_dim, dim), (wv, kv_dim, dim),
            (wo, dim, dim), (w1, hidden_dim, dim), (w2, dim, hidden_dim),
            (w3, hidden_dim, dim)):
        per_layer = rows_per_layer * cols
        for layer in range(n_layers):
            quantize_rows(body, matrix[layer * per_layer: (layer + 1) * per_layer],
                          rows_per_layer, cols, group_size)

    header = struct.pack("<II7iBi", RUN_Q_MAGIC, RUN_Q_VERSION,
                         dim, hidden_dim, n_layers, n_heads, n_kv_heads,
                         vocab_size, seq_len, 1, group_size)
    header += b"\0" * (RUN_Q_HEADER_SIZE - len(header))

    with open(dst_path, "wb") as dst:
        dst.write(header)
        dst.write(body)
    print("dim=%d hidden=%d layers=%d heads=%d kv_heads=%d vocab=%d seq_len=%d"
          % (dim, hidden_dim, n_layers, n_heads, n_kv_heads, vocab_size,
             seq_len))
    print("wrote %s: %d bytes (group_size=%d)" % (dst_path, len(header) + len(body), group_size))


if __name__ == "__main__":
    main()
