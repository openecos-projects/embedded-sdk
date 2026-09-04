# GPIO 基础示例

该示例使用公共 GPIO Driver 演示以下基本操作：

- 将 GPIO 配置为普通输入或输出模式；
- 读取输入引脚的高、低电平；
- 将读取到的电平写入输出引脚；
- 检查 GPIO API 返回的错误码。

当前示例使用 StarrySky L4 板载资源：

| 用途 | 引脚 | 电平语义 |
| --- | --- | --- |
| 输入 | GPIO1[7]（按键 0） | 按下为低电平 |
| 输出 | GPIO1[5]（LED 0） | 低电平点亮 |

因此，按下按键 0 时 LED 0 点亮，松开时 LED 0 熄灭。串口仅在输入电平变化时打印当前输入和输出电平。

创建并构建工程：

```bash
ecos project create gpio-basic --board starrysky-l4
cd gpio-basic
ecos build
```

在其他板卡上使用时，请根据板卡原理图修改 `GPIO_DEMO_PORT`、`GPIO_INPUT_PIN` 和 `GPIO_OUTPUT_PIN`。
