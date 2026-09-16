# AT24C64 EEPROM 读写示例

该示例通过公共 I2C Driver 驱动 AT24C64 EEPROM（7 位地址 `0x50`，容量
8 KiB，页 32 字节）：初始化总线后探测器件，随后从地址 `0x0010` 写入
40 字节已知图样（故意跨越一个页边界以验证驱动的分页写入），读回后逐字节
比对，并通过串口打印写入/读回的 hex dump 与 PASS/FAIL 结果。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 控制器 |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 I2C0 |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 不支持 | 板级未引出 I2C |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 不支持 | 板级未引出 I2C |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 支持 | CL1-2512 I2C0 |

当前示例要求板卡提供 `console`、`i2c-bus` 资源，并要求对应 Target 支持 I2C。
StarrySky L4C2/L4C3 未在板级引出 I2C 总线，示例清单已通过
`unsupported_boards` 显式排除这两块板卡，创建工程时会被拒绝。

## 创建和构建

创建并构建工程：

```bash
ecos project create i2c-at24c64 --board starrysky-l4-c1
cd i2c-at24c64
ecos build
```

使用 StartySky T1-Pico 时，将板卡参数改为 `--board t1-pico`。

## 连接和运行

ysyx-2512-1 的 I2C0 SCL 复用到 GPIO0[27]、SDA 复用到 GPIO0[28]。AT24C64
接至该总线（A2/A1/A0 接地时地址为 `0x50`）后即可运行。串口输出形如：

```text
written:
0010: a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af
0020: b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf
0030: c0 c1 c2 c3 c4 c5 c6 c7
readback:
0010: a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af
0020: b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf
0030: c0 c1 c2 c3 c4 c5 c6 c7
[i2c-at24c64] PASS: 40 bytes verified
```
