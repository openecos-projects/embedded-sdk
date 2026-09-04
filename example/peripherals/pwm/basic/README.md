# PWM 基础示例

该示例使用公共 PWM Driver 控制一路 PWM，演示：

- 配置 PWM 控制器的时钟分频和周期；
- 配置并动态修改单个通道的占空比；
- 启动和停止 PWM 输出；
- 为配置、占空比变化和启停操作输出日志。

当前示例使用 StarrySky L4 的 PWM0 通道 0，输出引脚为 `GPIO1[14]`。运行后每隔
2 秒依次切换以下状态：

1. 25% 占空比并启动输出；
2. 运行中将占空比更新为 75%；
3. 停止输出。

创建并构建工程：

```bash
ecos project create pwm-basic --board starrysky-l4
cd pwm-basic
ecos build
```

可以用示波器或逻辑分析仪观察 `GPIO1[14]`。在其他 Target 上使用前，请先查询 PWM
实例数量，并根据板卡原理图确认通道对应的输出引脚。
