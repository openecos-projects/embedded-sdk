# 用户 BSP 目录

`ecos board import <BSP目录>` 会把检查通过的外部 BSP 复制到本目录：

```text
board/UserBSP/<BSP id>/
```

请优先使用 `ecos board import`、`ecos board add` 和 `ecos board remove` 管理这里的内容，不要手工修改已经导入的固定版本。外部 BSP 的编写方法见 `docs/bsp_import.md`。
