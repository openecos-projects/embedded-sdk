# TM1650 四位数码管示例

该示例通过 GPIO 模拟 TM1650 两线协议，驱动板载 TM1650-SOP16 及两片
GS2020CR-G 数码管（共 4 位，段 A～G + 小数点 DP）。程序初始化后以 100 ms
步进在数码管上循环显示 `0..9999` 十进制计数。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 信号连接 |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | SEG_DAT=GPIO1[9]，SEG_CLK=GPIO1[10] |

当前示例要求板卡在 `ecos-board.yml` 中提供 `console`、`tm1650` 资源，
目前只有 StarrySky L4C1 声明了 `tm1650` 资源（SEG_DAT=GPIO1[9]、
SEG_CLK=GPIO1[10]，各经 10 kΩ 上拉至 3V3），其余板卡创建工程时会被拒绝。

## 创建和构建

创建并构建工程：

```bash
ecos project create gpio-tm1650 --board starrysky-l4-c1
cd gpio-tm1650
ecos build
```

## 连接和运行

TM1650 的两线协议为"类 I2C"，电平/时序与标准 I2C 控制器不兼容，驱动采用
GPIO 开漏模拟（输出低电平/释放输入两种状态，依靠外部上拉电阻拉高）。
运行后数码管从 `0` 开始递增计数，串口输出：

```text
[gpio-tm1650] TM1650 ready (DAT=GPIO1[9], CLK=GPIO1[10]), counting
```
