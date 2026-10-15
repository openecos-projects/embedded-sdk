# LLM 故事续写示例（llm-chat）

在 T1-Pico（cl1-2512，RV32IMC，无 FPU）上运行 TinyStories-260K 的
int8 量化推理：串口控制台输入一句英文开头，模型以打字机方式流式续写故事。
启动时经 `driver-rcu` 把 CPU/AXI/AHB 切到 PLL（默认 CPU 200 MHz，APB 保持
20 MHz 故串口波特率不变），相比 20 MHz 基线推理提速约 10 倍。
推理核心 `runq.c` vendor 自 [llama2.c](https://github.com/karpathy/llama2.c)
（MIT License, Copyright (c) 2023 Andrej Karpathy），为裸机环境做了适配
（见文件头注释的改动清单）。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 说明 |
| --- | --- | --- | --- |
| StarrySky T1-Pico | `starrysky-t1-pico` 或 `t1-pico` | 支持 | 权重 XIP 于 Flash，KV cache 等经堆组件分配到 32M SDRAM |
| StarrySky L4 系列 | — | 不支持 | 堆池尚未在 ysyx-2512 启动链注册（清单已显式排除） |
| StarrySky L3.1 | — | 不支持 | 仅有基础 RCU，且没有 LLM 所需的启动堆池（清单已显式排除） |

## 准备模型文件

把下面两个文件放到本示例目录（`examples/ai/llm-chat/`）或工程目录
（`ecos project create` 后的目录）；它们不进仓库：

```bash
# 词表（tokenizer，512 词）
curl -fSL -o tok512.bin \
  https://hf-mirror.com/karpathy/tinyllamas/resolve/main/stories260K/tok512.bin

# fp32 权重
curl -fSL -o stories260K.bin \
  https://hf-mirror.com/karpathy/tinyllamas/resolve/main/stories260K/stories260K.bin

# 量化为 int8（仅依赖 Python3 标准库，产出约 274 KiB 的 stories260k_q8.bin）
python3 quantize_q8.py stories260K.bin stories260k_q8.bin
```

huggingface.co 直连可用时把域名换成 `huggingface.co` 即可。

## 创建和构建

```bash
ecos project create llm-chat --board t1-pico
cd llm-chat
# 把 stories260k_q8.bin 与 tok512.bin 放进工程目录
ecos build
```

## 运行

烧录后打开串口（115200 8N1），出现提示符后输入一句英文开头并回车：

```text
[llm-chat] model ready: dim=64 layers=5 vocab=512, free SDRAM 33... B
TinyStories-260K story writer; type an opening line.
you> Once upon a time, a little cat
 found a red balloon under the old bridge. She picked it up and ran...
[llm-chat] [57 tokens]
you>
```

- 生成上限 `LLM_GEN_STEPS`（默认 160 token），`LLM_TEMPERATURE` /
  `LLM_TOP_P` / `LLM_RNG_SEED` 在 `main.c` 顶部调整；温度为 0 时是确定的
  贪心输出，方便板测复现。
- 该模型是**故事续写**而非对话模型：输入开场白、它接着编，不支持问答。
- 速度参考：CPU 200 MHz 下约 **0.8 tok/s**（经 CLINT mtime 实测，见下方
  "性能优化轨迹"）。想继续加压可在 `main.c` 调整 `llm_pll_config` 的 N 与
  分频（Fvco 限 500~1200 MHz，驱动会拒绝越界组合）；超出已验证组合
  （CPU 200 MHz）需先上板确认稳定性。
- 计时基准说明：板上**只有 CLINT mtime（5 MHz）可信**（经秒表校验）；
  Timer0 计数频率随时钟域变化，不能当墙上时钟。日志中 `tok/s` 与
  `[prof]` 分阶段耗时均基于 mtime。

## 实现要点

- `runq.c`：Llama-2 架构 int8 前向（Q8_0 分组量化，支持不满一组的尾组），
  RMSNorm/SwiGLU/softmax 保留 fp32（经工具链 libgcc 软浮点）。
- RoPE 的 cos/sin 在 `runq_init` 预计算成表（seq_len×head_size，32 KB
  SDRAM），前向纯查表；数值与逐点 powf/cosf/sinf 逐比特一致。
- softmax 与 SwiGLU 的 `expf` 用 `fast_expf` 替代（指数位构造 + 4 次
  多项式，全域相对误差约 1e-4）；注意力打分的 `sqrtf(head_size)` 每前向
  只算一次，softmax 归一化与打分缩放均为"一次除法换 n 次乘法"。
- 内存分配：小型激活缓冲在片上 SRAM（INTERNAL 池），KV cache、反量化
  嵌入表、RoPE 表、词表等在 32M SDRAM（EXTERNAL 池）。
- 权重与词表由 CMake 生成 `.incbin` 汇编并链接进 `.rodata`，随固件烧录进
  Flash；`runq_init` 默认把约 279 KB 权重体整体搬迁到 SDRAM
  （`RUNQ_WEIGHTS_IN_SDRAM`，置 0 回退 XIP 直读），避免每个 token 的权重
  流读占用 QSPI Flash 带宽。
- matmul/rmsnorm/softmax/quantize/fast_expf 等热点函数经
  `RUNQ_HOT`（`.isram.text` 段，`RUNQ_CODE_IN_ISRAM` 置 0 回退）放入
  ISRAM 运行，消除热点循环的 Flash XIP 取指。
- top-p 采样不做全量排序：每轮选出剩余最大值进前缀，累积概率超过 topp
  即停（O(k·n) 替代 O(n²) 插入排序）。

## 性能优化轨迹（T1-Pico 实测，CLINT mtime 计时）

同一 prompt、40 token 生成、CPU 200 MHz：

| 版本 | 总时长 | tok/s | 关键变化 |
| --- | --- | --- | --- |
| 初始（仅 PLL+权重搬 SDRAM+ISRAM） | ~172 s | ~0.15 | [prof] 显示 rope/sample 占 96%，之前所有 matmul 优化都打偏 |
| + RoPE 预计算表 + top-p 去排序 | 89.0 s | 0.4 | rope 306s→1.7s，sample 194s→6.5s |
| + sqrtf 外提 + 除法变乘法 | 48.4 s | 0.8 | attn 52.6s→14.1s（重复软浮点开方是元凶） |
| + matmul 按字打包（dot4） | 50.1 s | 0.7 | **负优化，已回退**：rv32imc 无 sext.b，拆包指令开销超过访存节省；证明瓶颈在核内指令数而非访存宽度 |

最终每 token 约 1210 ms 的分布（`[prof]`，mtime）：attn 37%、matmul 36%、
sample 13%、rope 4%、其余（SwiGLU/rmsnorm/fcopy）约 10%。

## 后续优化方向（按已量化的 ROI 排序）

1. **硬件 dot4 指令**：一条指令完成 4 路 int8 乘累加（替代 rv32imc 上
   ~26 条拆包指令）。matmul+attn 合计约 28 s/40 token，是可定点化部分
   的上限，也是该硬件立项的量化依据。
2. **attention 定点化**：attn 段 14 s 的大头是软浮点 MAC 与 KV cache
   的 SDRAM 访问；KV cache 存 int8+scale 可复用 matmul 的整数路径，
   但精度风险与工程量高于前述各项。
3. **FPU**：消除全部软浮点 libcall，但面积成本最高。

## PC 对照环境

`testdir/pc-ref/`（不进仓库的测试区）提供两级对照：上游 llama2.c `run.c`
（fp32 金标准）与把本目录 `runq.c` 原样编译到 PC 的 `runq-sim`。贪心输出
曾用于逐字节回归（RoPE 表、权重搬迁、top-p 选择均无差异）；引入
`fast_expf` 近似后输出在概率接近处会分叉，属预期行为（质量等价）。
