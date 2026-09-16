# PWM 蜂鸣器示例

该示例通过公共 PWM Driver 驱动板载无源蜂鸣器（额定 4 kHz，S8050 三极管
驱动，1N4148W 续流）：先以额定频率 4 kHz 鸣响 500 ms，随后播放一个八度的
C 大调音阶，循环演示。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 驱动信号 |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | PWM0 通道 0（GPIO1[14]） |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 支持 | PWM0 通道 0（GPIO1[14]） |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 支持 | PWM0 通道 0（GPIO1[14]） |

当前示例要求板卡提供 `console`、`pwm-output` 资源，L4 全系列均已声明。

## 创建和构建

创建并构建工程：

```bash
ecos project create pwm-buzzer --board starrysky-l4-c1
cd pwm-buzzer
ecos build
```

使用 L4C2/L4C3 时，将板卡参数改为 `--board l4c2` 或 `--board l4c3`。

## 连接和运行

蜂鸣器为板载器件，无需接线。驱动按 `频率 = PWM 输入时钟（默认 50 MHz）
/ 分频系数 / 周期` 计算音调时序，占空比固定 50%。运行后串口输出：

```text
[pwm-buzzer] buzzer ready, rated tone 4000 Hz
[pwm-buzzer] playing scale
...
```

`ECOS_BUZZER_CONFIG_DEFAULT` 对应 PWM0 通道 0；若 PWM 输入时钟不是
50 MHz，修改 `ecos_buzzer_config_t.pwm_clock_hz` 即可。
