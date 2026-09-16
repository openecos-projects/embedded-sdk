# ESP01S AT 模块示例

该示例通过统一 UART 公共 Driver 的 `ECOS_UART_PORT_1`（hp_uart 块，
TX=GPIO0[25]，RX=GPIO0[26]）驱动 ESP01S WiFi 模块：完成 AT 握手并关闭
回显后，查询并打印 AT 固件版本，随后设置为 Station 模式、单连接模式。
在 `main.c` 顶部填入 `WIFI_SSID` / `WIFI_PASSWORD` 后，还会执行入网
（`AT+CWJAP`）；留空则跳过入网步骤。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | UART |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | UART1（hp 块） |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 支持 | UART1（hp 块） |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 支持 | UART1（hp 块） |

当前示例要求 Target 具备 `hp-uart` 能力且板卡提供 `console` 资源，
ysyx-2512-1 与 ysyx-2512-2 均已声明该能力，L4 全系列可用。

## 创建和构建

创建并构建工程：

```bash
ecos project create uart-esp01s --board starrysky-l4-c1
cd uart-esp01s
ecos build
```

使用 L4C2/L4C3 时，将板卡参数改为 `--board l4c2` 或 `--board l4c3`。

## 连接和运行

ESP01S 接至 UART1：模块 RX 接 GPIO0[25]（UART1 TX），模块 TX 接
GPIO0[26]（UART1 RX），注意电平匹配（ESP01S 为 3.3V 器件）并共地。
ESP01S 出厂波特率通常为 115200 8N1，驱动默认按此配置；不同时可在
`main.c` 中修改 `ECOS_ESP01S_CONFIG_DEFAULT` 的波特率。

未填入 WiFi 凭据时，串口输出形如：

```text
[uart-esp01s] AT handshake OK
[uart-esp01s] firmware version:
AT version:1.x.x.x(...)
SDK version:...
...
[uart-esp01s] station mode, single connection
[uart-esp01s] WIFI_SSID is empty, skipping wifi_join; edit main.c to join an access point
[uart-esp01s] done
```
