# Display Donut

在提供 ST7735 `display` 资源的板卡上绘制旋转的环面。例程只调用 BSP Console、
BSP Display 和 Timer 公开接口；引脚、QSPI 控制器及屏幕方向由 Board 资源决定。

```bash
python3 tools/ecos.py --sdk . project create display-donut --board starrysky-l3-1
python3 tools/ecos.py --sdk . build --project display-donut
```

L3.1 的显示接线来自旧时钟模板，实际面板接线及帧率需要实板验收。
