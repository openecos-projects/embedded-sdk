# GPIO 基础示例

该示例使用公共 GPIO Driver 演示以下基本操作：

- 将 GPIO 配置为普通输入或输出模式；
- 读取输入引脚的高、低电平；
- 将读取到的电平写入输出引脚；
- 检查 GPIO API 返回的错误码。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 说明 |
| --- | --- | --- | --- |
| StarrySky L4 | `starrysky-l4` 或 `l4` | 支持 | 已提供 `gpio-demo` 板级资源 |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 支持 | 已提供 `gpio-demo` 板级资源 |

当前示例要求板卡同时提供 `console`、`gpio-demo` 资源，并要求对应 Target 支持
GPIO。StartySky T1-Pico 的 LED 已确认为 `GPIOD4`、Button 已确认为 `GPIOA7`；
CL1-2512 已提供相应的 GPIO HAL。

## 引脚连接

当前示例使用以下板载资源：

| 板卡 | 输入 | 输出 | 输出电平语义 |
| --- | --- | --- | --- |
| StarrySky L4 | `GPIO1[7]`（按键 0） | `GPIO1[5]`（LED 0） | 低电平点亮 |
| StartySky T1-Pico | `GPIOA7`（Button） | `GPIOD4`（LED 0） | 低电平点亮 |

示例将输入引脚的原始电平直接写入输出引脚。对于低电平有效的按键，按下时 LED
点亮，松开时 LED 熄灭。串口仅在输入电平变化时打印当前输入和输出电平。

## 创建和构建

创建并构建工程：

```bash
ecos project create gpio-basic --board starrysky-l4
cd gpio-basic
ecos build
```

使用 StartySky T1-Pico 时，将创建命令中的板卡参数改为 `--board t1-pico`。
