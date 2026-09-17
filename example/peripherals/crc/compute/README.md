# CRC 计算示例

该示例使用公共 CRC Driver 以默认配置（初值 0xFFFF、最终异或 0、模式 2）
对 4 个固定的 32 位字计算硬件 CRC，并打印结果。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | IP |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 CRC |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 支持 | ysyx-2512-1 CRC |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 支持 | ysyx-2512-2 CRC |
| StarrySky T1-Pico | `starrysky-t1-pico` 或 `t1-pico` | 不支持 | CL1-2512 无此 IP |

当前示例要求板卡提供 `console` 资源，并要求对应 Target 具备 `crc` 能力；
CL1-2512（T1-Pico）未声明该能力，创建工程时会被拒绝。

## 创建和构建

```bash
ecos project create crc-compute --board starrysky-l4-c1
cd crc-compute
ecos build
```

使用 L4C2/L4C3 时，将板卡参数改为 `--board l4c2` 或 `--board l4c3`。

## 运行

无需接线，运行后串口输出形如：

```text
[crc-compute] CRC of 4 word(s): 0x........
```

结果取决于模式（多项式/位宽）；`main.c` 中可修改
`ECOS_CRC_CONFIG_DEFAULT` 的 `mode` 字段对比不同模式的输出。
