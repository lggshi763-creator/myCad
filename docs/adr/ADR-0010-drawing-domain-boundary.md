# ADR-0010: 工程图（Drawing）作为独立 Domain 聚合

- **Status**: Proposed
- **Date**: 2026-05-06
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / <人工待 review>

## Context（背景）

myCad 现有架构（[ARCHITECTURE.md](../../ARCHITECTURE.md)）覆盖 3D 建模链：sketch → feature → assembly。但**完全没有 2D 工程图模块**。

工程实际中：
- 装配车间、合同评审、ISO 9001 认证现场审查 — 全部要 2D 工程图作为正式可签署文件
- SolidWorks / Creo / Inventor 的 "Drawing module" 是核心模块、不是附加项
- 国内军工 / 央企研发场景 — 90% 合同交付物形态是 PDF 工程图（不是 STEP/IGES）

如果 myCad **只能输出 3D 模型不能给 2D 工程图**，等于在工业 B 端市场被废。这是 P0 级缺失。

不决策的代价：到 Phase 1 末（v0.1 公开发布）才发现"差一个核心模块"，需要回炉重构 → 滑期 6 个月以上。

## Decision（决策）

**Drawing 作为独立的 Domain 聚合（aggregate）**，存活在 `src/domain/drawing/` 子模块，与 sketch / feature / assembly **同级**而非嵌套于其中。

**关键不变量：**
1. **Drawing 通过 AggregateId 弱引用 3D 模型**，不嵌入 / 不持有指针 / 不耦合具体 3D 几何
2. **Drawing 自身是事件溯源的聚合根**，新事件类型独立（不复用 sketch / feature 的事件命名空间）
3. **Drawing 的事件存储和 3D 模型的事件存储可以分库**（同一 SQLite EventStore 但不同 stream）

**新增 5 个核心事件：**

| 事件 | 含义 |
|---|---|
| `SheetCreated` | 创建一份图纸（含图框、标题栏、单位制、比例） |
| `ViewProjected` | 在某 sheet 上投影一个视图（正三视图 / 等轴 / 剖视 / 局部放大），引用 3D 模型 ID |
| `DimensionPlaced` | 在某 view 上放置一个尺寸标注（线性 / 角度 / 直径 / 半径） |
| `AnnotationAdded` | 在某 view 上添加注释（文字 / 形位公差符号 / 表面粗糙度） |
| `BomGenerated` | 装配体 → 明细栏（BOM）抽取，引用装配 AggregateId |

我们决定 **Drawing 作为独立聚合 + 弱耦合 3D 引用**，而不是 **将 Drawing 作为某个 3D 模型的子部分 / "view" 视图**。

## Considered Alternatives（候选方案）

### Option A: Drawing 作为独立聚合，通过 ID 引用 3D 模型 ✅ **选这个**

- **描述**：`drawing/` 与 `sketch/feature/assembly/` 同级；Drawing 持有目标 3D 模型的 `AggregateId`，不嵌入几何
- **优点**：
  - 3D 模型可以独立演化，drawing 版本独立追踪
  - Drawing 流可以并行编辑（多人同时改一份图纸）而不阻塞 3D 模型
  - 一个 3D 模型可以挂多份 drawing（不同客户 / 不同语言版本）
  - 同一 drawing 可以引用多个 3D 模型（典型的装配图）
  - 符合 DDD 聚合边界原则（aggregate 之间只通过 ID 引用）
- **缺点**：
  - 跨聚合一致性弱 — 3D 模型改了，drawing 不会自动重投影；需要显式触发 `ViewProjectionRequested` 命令
  - "drawing 引用了一个不存在的 3D 模型 ID" 这种错误要在 application 层校验，domain 层不阻止

### Option B: Drawing 作为 3D 模型聚合的子部分

- **描述**：每个 3D 模型 aggregate 内部带一个 `Drawings` 集合
- **优点**：
  - 强一致性：3D 模型变 → drawings 自动失效
- **缺点**：
  - 一个 drawing 不能引用多个 3D 模型（装配图卡死）
  - 所有 drawing 操作都要走 3D 模型 aggregate 的事件流，单点写入瓶颈
  - 违反 DDD 聚合大小建议（聚合应该 small + cohesive）
- **拒绝理由**：装配图必备，被这条 deal-breaker 直接否决

### Option C: Drawing 不是聚合，是"视图层产物"（view layer artifact）

- **描述**：每次需要 drawing 时由 application 层从 3D 模型重新生成，不持久化
- **优点**：实现简单，无新事件类型
- **缺点**：
  - 标注 / 注释 / 局部修改无处保存
  - 用户每次打开图纸都要重新计算 — 大装配 30+ 秒
  - 不能版本化（"上次给客户发的图是哪一版"无法回答）
- **拒绝理由**：CAD 工程图不是临时产物，必须可持久化、可版本化、可签字

## Rationale（理由）

- **硬约束**：BOM / 标注 / 工程师手工修订的视图都必须可保存；选 C 直接出局
- **DDD 原则**：聚合之间通过 ID 引用，不嵌入；选 B 违反此原则且压制装配图能力
- **演化压力**：未来 AI 识别图纸（[ADR-0012](ADR-0012-drawing-recognition-strategy.md)）会反向生成 drawing 事件流，独立聚合更好接入
- **杀手特性**：选 A 后，drawing 模块可以**完全独立于 3D 实现进度**开发 —— Sprint 0.2 后期可以并行启动，不必等 OCCT adapter 就绪

## Consequences（后果）

### Positive

- ✅ Drawing 模块可以**独立 sprint 推进**，与 3D 模型解耦
- ✅ 一份 drawing 可引用多个 3D 模型 → 装配图自然支持
- ✅ Drawing 事件流独立 → 可针对 drawing 场景单独优化（投影是计算密集型，可异步）
- ✅ 多客户场景下，drawing 可分租户存储，3D 模型保持单一真相

### Negative

- ⚠️ 跨聚合一致性弱 — 必须在 application 层加 `ViewProjectionInvalidated` 这类协调事件，否则 3D 模型改了 drawing 显示陈旧（缓解：CommandBus 拦截 3D 模型修改命令时同步发出 invalidate 事件）
- ⚠️ Drawing 引用悬空（dangling AggregateId）的可能 — 3D 模型被删除时 drawing 仍存在；需要软删除策略 + tombstone 事件
- ⚠️ 投影是计算密集型（OCCT 的 HLR 模块单线程一份图 5-30 秒）— 必须异步化，drawing 的"渲染"事件流和"编辑"事件流分开

### Neutral

- 🔧 需要新增 [ADR-0011](ADR-0011-dxf-library-selection.md)（DXF/DWG 库选型）
- 🔧 需要新增 [ADR-0012](ADR-0012-drawing-recognition-strategy.md)（识别走 VLM 还是本地）
- 🔧 [docs/architecture/06-roadmap.md](../architecture/06-roadmap.md) 需要插入 Phase 1.C / 1.E（生成）和 Phase 2.B / 2.D（识别）
- 🔧 [src/domain/drawing/](../../src/domain/drawing/) 模块骨架需要落地

## Implementation Notes（实施注记）

### 目录结构

```
src/domain/drawing/
├── CMakeLists.txt
├── shared/include/mycad/domain/drawing/
│   ├── Drawing.hpp                     ← 聚合根
│   ├── Sheet.hpp                       ← 图纸（含图框 / 标题栏）
│   ├── View.hpp                        ← 视图（正视 / 俯视 / 等轴 / 剖视 / 局部放大）
│   ├── Dimension.hpp                   ← 尺寸标注（线性 / 角度 / 直径 / 半径）
│   ├── Annotation.hpp                  ← 注释（文字 / 形位公差 / 表面粗糙度）
│   ├── Bom.hpp                         ← 明细栏
│   ├── events/
│   │   ├── SheetCreated.hpp
│   │   ├── ViewProjected.hpp
│   │   ├── DimensionPlaced.hpp
│   │   ├── AnnotationAdded.hpp
│   │   └── BomGenerated.hpp
│   └── ports/
│       ├── IDrawingProjectionPort.hpp  ← 3D → 2D 投影接口（infrastructure 实现）
│       └── IDrawingExportPort.hpp      ← DXF / PDF 导出接口
└── (仅头文件 + minimal placeholder.cpp，无 OCCT/Qt 引用 — 遵守 ADR-0002)
```

### 测试策略

- 单测覆盖 ≥ 90%：值对象（View / Dimension / Annotation）+ 聚合不变量（Drawing 不允许 Sheet 编号重复 / View 必须引用存在的 Sheet）
- 集成测试 Phase 1.C 落地后再加（涉及 OCCT 的 HLR 投影）

### 迁移步骤

无 — 这是新增模块，不影响现有代码。

## References（参考）

- 相关 ADR: ADR-0002（domain 零依赖）, ADR-0003（事件不可变）, ADR-0011（DXF 库）, ADR-0012（识别策略）
- 提案来源: [docs/proposals/2026-W19-expansion.md §2](../proposals/2026-W19-expansion.md)
- 相关 roadmap 改动: Phase 1.C / 1.E / 2.B / 2.D（[docs/architecture/06-roadmap.md](../architecture/06-roadmap.md)）
- 外部参考: SolidWorks Drawing 模块、Creo Drawing、AutoCAD 工程图标注规范 GB/T 17450-1998
