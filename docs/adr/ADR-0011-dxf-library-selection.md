# ADR-0011: DXF / DWG 库选型

- **Status**: Proposed
- **Date**: 2026-05-06
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / <人工待 review>

## Context（背景）

[ADR-0010](ADR-0010-drawing-domain-boundary.md) 决定 drawing 作为独立 domain 聚合，**Phase 1.C** 要交付 drawing 生成 MVP。生成出来的图纸最终要落到磁盘，最低必备的格式：

- **DXF**（AutoCAD Drawing Exchange Format）— ASCII / Binary 文本格式，工业事实标准，所有 CAD 软件都能读
- **DWG**（AutoCAD 私有二进制格式）— 装机量更大但格式封闭、Autodesk 不公开规范
- **PDF** — 输出（不需要回读），PDF 库 vcpkg 有现成（HPDF / PoDoFo）

DWG 的开放实现（LibreDWG）协议是 **GPL-3.0**，与 myCad 的 **LGPL-3.0** 不兼容（GPL "传染" → 一旦链接 LibreDWG，整个 myCad 必须 GPL，违反 [ADR-0001](ADR-0001-license-lgpl-3.md) 的商业插件友好策略）。

不决策的代价：Phase 1.C 启动时再纠结，或更糟 — 集成了 GPL 库后回头发现协议冲突，回退成本高。

## Decision（决策）

我们决定 **第一阶段只支持 DXF（读 + 写），通过 `libdxfrw` 库；不支持 DWG**。需要 DWG 的用户**外部转**（ODA File Converter / AutoCAD Save As DXF / Aspose.CAD）。

PDF 输出走 **Qt 的 Print Support 模块**（Qt6::PrintSupport），不引入新依赖。

## Considered Alternatives（候选方案）

### Option A: libdxfrw 仅做 DXF ✅ **选这个**

- **描述**：使用 `libdxfrw`（LGPL-3.0，纯 C++）做 DXF 读写；DWG 不做
- **优点**：
  - **协议兼容** — LGPL 与 myCad LGPL-3.0 协议完全一致
  - 纯 C++、无第三方运行时依赖（除标准库）
  - 维护活跃（LibreCAD 项目核心，2010+ 持续维护）
  - 已在 vcpkg（`libdxfrw` port），集成成本 = 0
  - DXF 是事实工业标准 — 用户在 AutoCAD / SolidWorks / Creo 都能读
- **缺点**：
  - 用户拿到 DWG 文件需要外部转（额外流程）
  - libdxfrw 对 DXF 高级特性（动态块、参数化标注）支持有限 — 但 myCad 输出的 DXF 不需要这些
- **适用场景**：所有 myCad 用户，Phase 1.C-1.E

### Option B: LibreDWG（开源 DWG 实现）

- **描述**：使用 `libredwg`（GPL-3.0+）做 DWG 读写
- **优点**：
  - 直接支持 DWG，用户体验最好
  - 唯一可用的开源 DWG 实现
- **缺点**：
  - **协议冲突致命** — GPL "传染" 到整个 myCad → 商业插件不能闭源 → 违反 [ADR-0001](ADR-0001-license-lgpl-3.md)
  - 即便单独编译为可执行（"separate process"）规避传染，体验劣化、增加架构复杂度
  - DWG 格式封闭，LibreDWG 兼容性比 DXF 差，新版 AutoCAD DWG 经常解析失败
- **拒绝理由**：协议冲突 deal-breaker

### Option C: ODA SDK（Open Design Alliance，商业）

- **描述**：使用 ODA Drawings SDK（商业授权 ~$5000/年/开发者）
- **优点**：
  - DWG 兼容性最强（ODA 是 DWG 反向工程 reference 实现）
  - 同时支持 DXF / DWG / DGN 等
- **缺点**：
  - 商业许可贵
  - 协议是 ODA 自有 EULA，不是开源协议
- **拒绝理由**：myCad 开源项目阶段不商用付费库；可能在未来 Phase 2 商业插件中由插件作者自行集成

### Option D: OCCT 自带的 DXF 模块（DEXchangeKit）

- **描述**：OpenCASCADE 自带 DEXchangeKit，能读写 DXF
- **优点**：
  - 已有依赖，无需新增
- **缺点**：
  - DXF 实现停留在 R12 / R14（约 1990 年代版本），现代 AutoCAD 输出的 DXF 经常解析不全
  - OCCT 的 DXF 是"3D 实体导入导出"思维，缺少 layer / lineweight / annotation 等 2D 工程图原生概念
  - 与 myCad 的 drawing domain 模型不匹配（OCCT 把 DXF 当 3D 数据，但工程图本质是 2D + 标注语义）
- **拒绝理由**：能力不匹配；用 OCCT DXF 还要写一个适配层把它转成 myCad 的 drawing 事件，反而更复杂

### Option E: 自研 DXF reader/writer

- **描述**：从 DXF 规范（公开）自己写解析器
- **优点**：完全可控
- **缺点**：DXF 规范 1300+ 页（[Autodesk DXF Reference](https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-235B22E0-A567-4CF6-92D3-38A2306D73F3)），自研 ≥ 6 人月
- **拒绝理由**：投资回报率低，已有 libdxfrw 不重复造轮子

## Rationale（理由）

- **硬约束**：协议必须 LGPL-3.0 兼容，淘汰 B / C
- **匹配度**：domain 模型是 2D + 标注，淘汰 D（3D 思维）
- **投资回报**：自研 vs. 复用 — E 高成本低收益，淘汰
- **杀手特性**：A 在 vcpkg 直接可用、纯 C++、维护活跃、协议匹配 — 4 项全中

## Consequences（后果）

### Positive

- ✅ 协议干净 — myCad 整个保持 LGPL-3.0，商业插件可以正常闭源
- ✅ 开发节奏快 — 不需要等待商业谈判 / 不需要反向 DWG
- ✅ 用户教育成本可接受 — DXF 是工业标准，"用 AutoCAD 转一下"用户能理解

### Negative

- ⚠️ DWG 用户需外部转换工具 — 缓解：在文档 [docs/operations/file-formats.md](../operations/file-formats.md)（待写）列出 ODA File Converter（免费）和 AutoCAD True View（免费）的下载地址 + 命令行用法
- ⚠️ 未来如果商业用户强烈要求原生 DWG — 路径：作为商业插件由插件作者自费集成 ODA SDK，不污染主仓
- ⚠️ libdxfrw 对 R2018+ DXF 的某些新特性可能不支持 — 缓解：跑通后写一份"已验证 AutoCAD 兼容性矩阵"持续维护

### Neutral

- 🔧 `vcpkg.json` 增加 `libdxfrw` 依赖，无 sub-feature 需要勾选
- 🔧 Qt Print Support 已被 qtbase 默认特性涵盖，无需新增依赖
- 🔧 [docs/architecture/04-tech-decisions.md](../architecture/04-tech-decisions.md) 需要追加 DXF 选型决策行

## Implementation Notes（实施注记）

### vcpkg.json 改动

```diff
   "dependencies": [
     "opencascade",
     {
       "name": "qtbase",
       "default-features": false,
-      "features": ["gui", "widgets", "opengl"]
+      "features": ["gui", "widgets", "opengl", "printsupport"]
     },
     "qttools",
+    "libdxfrw",
     "entt",
     ...
   ]
```

### 集成位置

- DXF 读：`src/infrastructure/drawing/DxfReader.{hpp,cpp}` — 实现 `mycad::domain::drawing::IDrawingExportPort` 的反向（"导入"端口待定，可能下个 sprint 再加）
- DXF 写：`src/infrastructure/drawing/DxfWriter.{hpp,cpp}` — 实现 `IDrawingExportPort::exportToDxf(...)`
- PDF 写：`src/infrastructure/drawing/PdfRenderer.{hpp,cpp}` — 用 Qt6::PrintSupport（位于 `src/infrastructure/` 而非 `src/ui/`，因为它是输出能力，不是 UI）

### 测试策略

- 写一个"基准 DXF" — 由 AutoCAD 2024 / SolidWorks 2024 / FreeCAD 1.x 各自导出一份相同图纸，作为 fixture
- DxfWriter 写出的文件要被这三个软件**正确读取**才算 PASS
- DxfReader 反过来读这三份 fixture，恢复出的 drawing 聚合内容应一致

### 迁移步骤

无 — 新功能。未来如果发现 libdxfrw 不够用，可以在 infrastructure 层切换 reader / writer 实现而不动 domain。

## References（参考）

- 相关 ADR: [ADR-0001](ADR-0001-license-lgpl-3.md)（LGPL）, [ADR-0010](ADR-0010-drawing-domain-boundary.md)（drawing domain）
- 提案来源: [docs/proposals/2026-W19-expansion.md §2](../proposals/2026-W19-expansion.md)
- libdxfrw: <https://github.com/LibreCAD/libdxfrw>
- DXF 规范: <https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-235B22E0-A567-4CF6-92D3-38A2306D73F3>
- ODA File Converter（免费 DWG↔DXF 工具）: <https://www.opendesign.com/guestfiles/oda_file_converter>
