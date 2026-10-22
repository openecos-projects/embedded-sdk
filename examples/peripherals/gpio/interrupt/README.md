# GPIO 双边沿中断示例

例程名：`gpio-interrupt`。当前兼容板卡：StarrySky T1-Pico（`starrysky-t1-pico`）。
要求 `gpio-demo` 板级资源和 GPIO 中断能力；不能仅因其他板卡提供普通 GPIO 就认为兼容。
程序使用生成的 `ECOS_BOARD_GPIO_DEMO_INPUT`，不在应用中硬编码引脚。

## 接线与准备

T1-Pico 当前输入资源为板载 Button：`GPIOA7`，默认上拉、按下为低电平。
使用板载按钮即可；如改用外部信号，应遵循板卡电气规格、共地且避免输入悬空，
禁止接入超出 GPIO 允许范围的电压。示例不驱动 LED。

## 创建、构建和运行

```bash
ecos project create gpio-interrupt --board starrysky-t1-pico
cd gpio-interrupt
ecos build
ecos flash
ecos monitor
```

控制台默认 115200 波特率；可用 `ecos monitor --port /dev/ttyUSB0` 指定串口。
CI 对应固件为 `gpio-interrupt-starrysky-t1-pico.bin`，按板卡下载流程烧录。

## 测试步骤与预期结果

1. 复位后确认出现 `GPIOA7 interrupt ready`。
2. 按下并松开按钮，串口应出现 `edges: N`，计数持续增加。
3. 重复操作至少 10 次，确认后续操作仍能使计数变化，无 panic 或意外复位。
4. 输入稳定时观察计数；若持续无操作增加，应检查抖动、干扰或输入电平。

程序设置 `ECOS_GPIO_INTERRUPT_ANYEDGE`，按下与释放都可触发。
没有软件消抖，机械按键一次操作可能产生多个边沿，不能要求每次按键恰好增加 1 或 2。
主循环也可能合并多次计数变化。精确边沿测试应使用满足电气要求的干净信号源。

只有 ready 而按钮操作无计数变化不能判为通过。
记录硬件版本、固件提交号/BIN SHA-256、输入接法、操作次数、计数变化及日志。
