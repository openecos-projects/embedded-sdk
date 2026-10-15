# Button 状态示例

使用板级 Button BSP 读取按键状态；只有状态变化时向 Console 打印。按键数量来自
`bsp_button_get_count()`，同一份源码可用于 T1-Pico 的单按键和 L4 C1/C2/C3 的
双按键配置。

```bash
ecos project create button-state --board starrysky-t1-pico
cd button-state
ecos build
```

按键极性与接线来自 Board 清单；实板动作与抖动仍需在人工验收时确认。
