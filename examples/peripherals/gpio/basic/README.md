# GPIO 基础示例

该示例使用公共 GPIO Driver 演示以下基本操作：

- 将 GPIO 配置为普通输入或输出模式；
- 读取输入引脚的高、低电平；
- 将读取到的电平写入输出引脚；
- 检查 GPIO API 返回的错误码。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 说明 |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | 已提供类型化 `gpio-demo` 板级资源 |
| StarrySky T1-Pico | `starrysky-t1-pico` 或 `t1-pico` | 支持 | 已提供类型化 `gpio-demo` 板级资源 |
| CH32V103C8T6 | `ch32v103c8t6` | 实验性 | PA0 输入、PA1 输出，需外接测试线 |

当前示例要求板卡同时提供 `console`、`gpio-demo` 资源，并要求对应 Target 支持
GPIO。StarrySky T1-Pico 的 LED 已确认为 `GPIOD4`、Button 已确认为 `GPIOA7`；
CH32V103C8T6 使用 PA0/PA1 作为外接测试点；CL1-2512 已提供相应的 GPIO HAL。示例源码不包含板卡名称或固定引脚号，配置阶段会从
Board 清单生成 `ecos/board_resources.h`。

## 引脚连接

当前示例使用以下板载资源：

| 板卡 | 输入 | 输出 | 输出电平语义 |
| --- | --- | --- | --- |
| StarrySky L4C1 | `GPIO1[7]`（按键 0） | `GPIO1[5]`（LED 0） | 低电平点亮 |
| StarrySky T1-Pico | `GPIOA7`（Button，默认上拉） | `GPIOD4`（LED 0） | 低电平点亮 |
| CH32V103C8T6 | `PA0`（外接输入） | `PA1`（外接输出） | 由测试线和仪器观察 |

示例将输入引脚的原始电平直接写入输出引脚。对于低电平有效的按键，按下时 LED
点亮，松开时 LED 熄灭。串口仅在输入电平变化时打印当前输入和输出电平。

## 创建和构建

创建并构建工程：

```bash
ecos project create gpio-basic --board starrysky-l4-c1
cd gpio-basic
ecos build
```

使用 StarrySky T1-Pico 时，将创建命令中的板卡参数改为 `--board t1-pico`。

CH32V103C8T6 测试：

```bash
ecos project create gpio-basic --board ch32v103c8t6
cd gpio-basic
ecos build
ecos flash
ecos monitor --port <串口设备>
```

将 PA0 接到 3.3 V 或 GND，观察串口中的输入变化，并用示波器观察 PA1 是否跟随。
不要将超过 3.3 V 的电平接入 GPIO；PA0/PA1 不是当前串口引脚。该示例只验证轮询
GPIO，不验证 GPIO 中断。
