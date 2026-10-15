# RCU PLL 时钟切换示例

该示例使用公共 RCU Driver 将 CPU/AXI/AHB 时钟域切换到 PLL（APB 保持
20MHz 原时钟，串口不受影响），切换前后分别回读并打印各时钟域频率与
CLK_CTRL，随后每 5 秒输出一次心跳，用于验证切时钟后系统持续存活。

默认配置 `ECOS_RCU_PLL_CONFIG_DEFAULT` 是已验证组合（N=40，SELECT×1，
OD=/2，分频 2/2/4，APB 保持）：Fvco=800MHz，Fpll=400MHz，CPU/AXI=200MHz，
AHB=100MHz，APB=20MHz。

注意：RCU 影响时钟与复位域，错误的配置可能导致系统异常；修改 PLL 参数前
请查阅数据手册，Fvco 必须落在 500~1200MHz。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | IP |
| --- | --- | --- | --- |
| StarrySky T1-Pico | `starrysky-t1-pico` 或 `t1-pico` | 支持 | CL1-2512 CRU/PLL |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 不支持 | ysyx-2512-1 无 PLL |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 不支持 | ysyx-2512-1 无 PLL |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 不支持 | ysyx-2512-2 无 PLL |
| StarrySky L3/L3.1 | `starrysky-l3`、`starrysky-l3-1` | 不支持 | 仅有基础 RCU，无 PLL |

当前示例要求板卡提供 `console` 资源，并要求对应 Target 具备 `rcu-pll` 能力；
无 PLL 的 Target 运行时 `ecos_rcu_pll_apply()` 返回 `ECOS_ERR_UNSUPPORTED`。

## 创建和构建

```bash
ecos project create rcu-pll --board starrysky-t1-pico
cd rcu-pll
ecos build
```

## 运行

无需接线，运行后串口输出形如：

```text
[rcu-pll] [TEST_START] switch CPU/AXI/AHB to PLL, keep APB unchanged
[rcu-pll] before: CLK_CTRL=0x........
[rcu-pll] PLL: 0 Hz
[rcu-pll] CPU: 20000000 Hz
...
[rcu-pll] after: CLK_CTRL=0x00001000
[rcu-pll] PLL: 400000000 Hz
[rcu-pll] CPU: 200000000 Hz
[rcu-pll] AXI: 200000000 Hz
[rcu-pll] AHB: 100000000 Hz
[rcu-pll] APB: 20000000 Hz
[rcu-pll] [TEST_PASS] PLL switched, heartbeat every 5 s
[rcu-pll] heartbeat 0
```
