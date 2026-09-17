# RCU 状态读取示例

该示例使用公共 RCU Driver 配置时钟分频比与控制位，随后回读并打印 STAT
状态寄存器，用于验证 RCU IP 的读写通路。

注意：RCU 影响时钟与复位域，`main.c` 中的 `RCU_CONTROL_BITS` 默认使用 2.x
冒烟验证过的 0xB；控制位含义由 SoC 定义，修改前请查阅数据手册，错误的
配置可能导致系统异常。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | IP |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 RCU |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 支持 | ysyx-2512-1 RCU |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 支持 | ysyx-2512-2 RCU |
| StarrySky T1-Pico | `starrysky-t1-pico` 或 `t1-pico` | 不支持 | CL1-2512 无此 IP |

当前示例要求板卡提供 `console` 资源，并要求对应 Target 具备 `rcu` 能力；
CL1-2512（T1-Pico）未声明该能力，创建工程时会被拒绝。

## 创建和构建

```bash
ecos project create rcu-status --board starrysky-l4-c1
cd rcu-status
ecos build
```

使用 L4C2/L4C3 时，将板卡参数改为 `--board l4c2` 或 `--board l4c3`。

## 运行

无需接线，运行后串口输出形如：

```text
[rcu-status] RCU status: 0x........ (divider 256, control 0xB)
```
