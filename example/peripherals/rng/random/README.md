# RNG 随机数示例

该示例使用公共 RNG Driver 以默认种子初始化随机数发生器，连续读取并打印
8 个 32 位随机字。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | IP |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 RNG |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 支持 | ysyx-2512-1 RNG |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 支持 | ysyx-2512-2 RNG |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 不支持 | CL1-2512 无此 IP |

当前示例要求板卡提供 `console` 资源，并要求对应 Target 具备 `rng` 能力；
CL1-2512（T1-Pico）未声明该能力，创建工程时会被拒绝。

## 创建和构建

```bash
ecos project create rng-random --board starrysky-l4-c1
cd rng-random
ecos build
```

使用 L4C2/L4C3 时，将板卡参数改为 `--board l4c2` 或 `--board l4c3`。

## 运行

无需接线，运行后串口输出形如：

```text
[rng-random] random[0] = 0x........
...
[rng-random] random[7] = 0x........
```

该硬件是可播种的伪随机源：相同种子产生相同序列，因此每次上电输出一致；
可在 `main.c` 中修改 `ECOS_RNG_CONFIG_DEFAULT` 的种子验证序列变化。
