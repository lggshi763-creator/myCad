# myCad 路线图

> 本文件是路线图的**摘要**。完整 sprint 级任务清单见 [docs/architecture/06-roadmap.md](./docs/architecture/06-roadmap.md)。

| Phase | 估时 | 目标 | 状态 |
|---|---|---|---|
| **Phase 0** 地基 | ~3 个月 | 架构骨架 + 3D 立方体 + 保存重开 | 🚧 In Progress |
| **Phase 1** MVP 草图建模 + 基础工程图 | ~7-9 个月 | 草图 → 拉伸 → 保存 → 撤销 → **2D 工程图导出 DXF/PDF**，v0.1 公开发布 | ⏳ Planned |
| **Phase 2** 完整 CAD + AI 识别 | ~18-24 个月 | 装配体 + 工程图增强 + **图纸识别（VLM）** + STEP/IGES/DXF + 插件市场 + AI v1，v1.0 发布 | ⏳ Planned |
| **Phase 3** 向 CAM/CAE 扩展 | 24+ 个月 | 第三方插件接入 CAM/CAE；架构演进 | ⏳ Planned |

---

## Phase 0 — 地基（当前）

### 一句话目标

能在窗口里看到一个 3D 立方体，鼠标能旋转视角，关闭重开依然能看到 — 同时架构骨架就位。

### Sprint 概览

| Sprint | 周次 | 主题 | 状态 |
|---|---|---|---|
| 0.1 | W1-W2 | 项目脚手架（CMake + vcpkg + CI + Visual Studio 配置） | 🚧 |
| 0.2 | W3-W4 | Domain 骨架（事件、聚合、Port 接口） | ⏳ |
| 0.3 | W5-W6 | Infrastructure Adapter（OCCT、EnTT、InMemoryEventStore） | ⏳ |
| 0.4 | W7-W8 | Application + 渲染端到端 | ⏳ |
| 0.5 | W9-W10 | Qt UI + SQLite EventStore + .mycad 文件 | ⏳ |
| 0.6 | W11-W12 | 插件骨架 + 内部 alpha | ⏳ |

### Phase 0 交付物

- ✅ 完整文档（架构、ADR、贡献指南）
- 🚧 可在 Windows / Linux / macOS 三平台编译通过
- ⏳ 能显示 3D 立方体 + 旋转视图
- ⏳ 能保存 / 重开 .mycad 文件
- ⏳ 内部 alpha 可邀请 1-2 个朋友试用

详见 [docs/architecture/06-roadmap.md §6.1](./docs/architecture/06-roadmap.md)

---

## Phase 1 — MVP 草图建模

### 一句话目标

完成"草图 → 拉伸 → 保存 → 重开 → 修改参数 → 联动更新 → 撤销重做"完整 9 步剧本，发布开源 v0.1。

### 主要主题

- **A 草图核心**（5 sprint）：Sketch BC、PlaneGCS 集成、绘制工具、拾取、状态反馈
- **B 特征建模**（4 sprint）：Feature BC、拉伸/旋转/切除、特征树 UI、参数联动
- **C 撤销 / 持久化**（2 sprint）：UndoRedoManager、快照策略
- **D 发布准备**（3 sprint）：STL/OBJ 导出、稳定性、发布

详见 [docs/architecture/06-roadmap.md §6.2](./docs/architecture/06-roadmap.md)

---

## Phase 2 — 完整 CAD

### 一句话目标

从 v0.1 个人玩具升级为 v1.0 可被独立设计师 / 小工坊真实使用的工具，社区生态初步成型。

### 核心功能目标

- 装配体（多零件文件引用、装配约束）
- 工程图（三视图、剖视图、DXF）
- 高级特征（倒角、圆角、抽壳、扫掠、放样）
- STEP/IGES/DXF 完善
- 插件市场基础设施 + Python 插件
- **AI 功能 v1**（自然语言→草图）
- 协同功能预研

### 生态里程碑

| 里程碑 | 目标时间 |
|---|---|
| 第一个外部 PR 合并 | Phase 2 W12 |
| 第一个第三方插件 | Phase 2 W24 |
| 插件市场上线 | Phase 2 W36 |
| 第一个付费用户 | Phase 2 W42 |
| 首个企业 PoC | Phase 2 W44+ |

详见 [docs/architecture/06-roadmap.md §6.3](./docs/architecture/06-roadmap.md)

---

## Phase 3 — 向 CAM/CAE 扩展

### 核心策略

**不自造 CAM/CAE 内核**。

- myCad 提供完整扩展点（Component 类型注册、System 注入、文件格式扩展）
- CAM 通过插件接入开源核（LinuxCNC、CAMotics）
- CAE 通过插件接入 OpenFOAM、Calculix
- myCad 价值 = "统一上下文" — 用户在同一软件里走完 CAD → CAM → CAE 流程

### 架构演进

详见 [docs/architecture/06-roadmap.md §6.4](./docs/architecture/06-roadmap.md) 与 [09-self-host-strategy.md](./docs/architecture/09-self-host-strategy.md)。

---

## 路线图维护

- 每个 Phase 末写 retrospective → `docs/retrospectives/phase-N.md`
- 重大调整必须发 [Discussion]，2 周窗口收集反馈
- 不接受"加塞紧急功能"打破 Phase 节奏 — 真正紧急的走 hotfix release

## 时间承诺免责声明

时间预估**仅是估算**，受以下因素影响：
- 个人开发者精力波动
- 上游依赖（OCCT、Qt 等）的 bug 与升级节奏
- 社区贡献速度
- 商业进展（私有化部署项目可能暂时挪用资源）

**承诺**：
- 严格按 Phase 顺序执行（不跳跃）
- 每月公开进度（GitHub Discussion）
- 估时偏差超过 50% 时主动写 retrospective 并调整剩余路线图

---

完整任务级路线图：[docs/architecture/06-roadmap.md](./docs/architecture/06-roadmap.md)
