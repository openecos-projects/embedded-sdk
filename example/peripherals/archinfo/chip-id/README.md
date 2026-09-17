# Archinfo 芯片标识示例

该示例使用公共 Archinfo Driver 读取并打印 SoC 的 32 位系统标识字和 64 位
芯片 ID（IDH:IDL 拼接）。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | IP |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 Archinfo |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 支持 | ysyx-2512-1 Archinfo |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 支持 | ysyx-2512-2 Archinfo |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 不支持 | CL1-2512 无此 IP |

当前示例要求板卡提供 `console` 资源，并要求对应 Target 具备 `archinfo`
能力；CL1-2512（T1-Pico）未声明该能力，创建工程时会被拒绝。

## 创建和构建

```bash
ecos project create archinfo-chip-id --board starrysky-l4-c1
cd archinfo-chip-id
ecos build
```

使用 L4C2/L4C3 时，将板卡参数改为 `--board l4c2` 或 `--board l4c3`。

## 运行

无需接线，运行后串口输出形如：

```text
[archinfo-chip-id] system id: 0x........
[archinfo-chip-id] chip id: 0x................
```

具体取值由 SoC 的 Archinfo 寄存器决定。
