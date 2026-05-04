# §九 第三方依赖隔离与长期自研路径

> 本章是 §四 决策清单的"长期主权"附录。
>
> myCad 短期严重依赖第三方库（OCCT、Qt、EnTT、PlaneGCS、FlatBuffers、OpenGL 等），这是个人开发者起步必然的取舍。但**架构必须从 Day 1 就为"未来可能的自研替代"留出隔离层与迁移路径** — 否则一旦上游出问题（停更、协议变化、商业转向、社区分裂），项目将陷入被动。
>
> 决策依据：[ADR-0009 — 第三方依赖隔离与长期自研路线](../adr/ADR-0009-self-host-roadmap.md)

## 9.1 设计哲学

### 9.1.1 三层依赖政策

```
┌────────────────────────────────────────────────────┐
│ Tier A — 核心依赖（必须有 Port 抽象 + 自研路径）        │
│   OCCT / Qt / EnTT / PlaneGCS / OpenGL / EventStore │
│   Adapter 严格隔离，向 Domain 层零泄漏                │
└────────────────────────────────────────────────────┘
              │
              ▼
┌────────────────────────────────────────────────────┐
│ Tier B — 工具依赖（不抽象，可直接用）                  │
│   Eigen / FlatBuffers / spdlog / fmt / Catch2 / SQLite │
│   工具型库，替换成本相对低，且自研无意义               │
└────────────────────────────────────────────────────┘
              │
              ▼
┌────────────────────────────────────────────────────┐
│ Tier C — 实用依赖（直接 #include，绑死也无所谓）        │
│   nlohmann/json, tomlplusplus, tl::expected, ULID  │
│   小型独立库，必要时复制到项目内即可（vendoring）      │
└────────────────────────────────────────────────────┘
```

### 9.1.2 隔离的"五层防火墙"

对于 Tier A 依赖，必须达到下列五层隔离：

1. **接口层**：定义 `IXxx` 纯虚接口（如 `IGeometryPort`），位于 Domain 层
2. **实现层**：第三方实现命名 `XxxAdapter`（如 `OcctGeometryAdapter`），位于 Infrastructure 层
3. **类型隔离**：第三方类型（OCCT `Handle`、Qt `QObject`、EnTT `entt::registry`）只在 Adapter 内出现
4. **依赖隔离**：CMake 中 Domain target 不允许 link 第三方库（编译期保证）
5. **测试隔离**：Domain 层单测用 Mock Adapter（如 `InMemoryGeometryPort`），不需要第三方运行时

### 9.1.3 何时考虑自研替代

**不替代的标准信号**（只要满足任一就继续用第三方）：
- 上游活跃（最近 6 个月有提交）
- 协议未变化
- API 稳定（至少同主版本兼容）
- 没有阻塞性 bug
- 自研估时 > 12 人月

**应替代的触发条件**（任一即应启动 ADR 评估）：
- 上游停更超过 12 个月
- 协议变更对商业模式有阻断
- 关键 bug 上游不修，自己 patch 已超过 5 处
- 性能瓶颈被库的设计天花板限制
- 出现核心商业价值无法绕开的技术诉求（必须改库内部）

---

## 9.2 各核心依赖的自研路径

### 9.2.1 OpenCASCADE → 自研 BRep 几何内核

**当前状态**：通过 `IGeometryPort` 接口完全抽象，OCCT 类型不出 `OcctGeometryAdapter` 边界。

**自研估时**：极高（**50-100 人月**） — 这是 CAD 行业最难的子系统

**短期不会启动**。但架构必须为此预留可能性。

#### 替代路径分阶段

| Phase | 目标 | 估时 | 触发条件 |
|---|---|---|---|
| **A 等价 wrap** | 现状 — `IGeometryPort` 完全 wrap OCCT | — | 默认 |
| **B 部分自研** | 用 CGAL / 自实现替代 OCCT 的某些部分（如简单原语生成） | 6 月 | 个别 OCCT 模块严重不可用 |
| **C 核心自研** | 自实现 BRep 数据结构 + 简单运算（基础 CAD 完全脱离 OCCT） | 24 月 | 商业版需要规避 OCCT LGPL；或 OCCT 真正停更 |
| **D 完全自研** | 含布尔、扫掠、放样、复杂曲面 | 60 月 | 极不可能（除非项目变成全职 + 多人团队） |

#### 自研启动后的迁移策略

- `IGeometryPort` 接口保持不变 → 上层零感知
- 新加 `MyBrepGeometryAdapter` 与 `OcctGeometryAdapter` 并存
- 一段时间内**双 Adapter 同时运行 + 结果对比**（diff testing）
- 通过率 > 99.9% 后切换默认 Adapter
- OCCT 留作可选 fallback 至少 2 个主版本

#### 当前应做的预防性工作

- [ ] `IGeometryPort` 接口设计**故意不要紧贴 OCCT 习惯** — 避免抽象成"OCCT 在 Domain 层的镜像"
- [ ] BRepHandle 用不透明 ID（已实现）→ 替换内核时不影响序列化格式
- [ ] 单测构建一组"几何运算金标准"（输入 + 期望输出）→ 替代时做回归

### 9.2.2 Qt → 自研 GUI 框架（或迁移其他）

**当前状态**：Qt 仅在 `src/ui/` 与 `src/infrastructure/qt-bridge/` 内出现。Domain / Application / 大部分 Infrastructure 与 Qt 无关。

**自研估时**：极高（**40-80 人月**） — 跨平台 GUI 极复杂

**也不会启动**，但有 Plan B。

#### 替代路径

| Phase | 目标 | 估时 | 触发条件 |
|---|---|---|---|
| **A 现状** | Qt 6 LTS | — | 默认 |
| **B 部分迁移** | UI 部分用 Qt，渲染面板用自实现 OpenGL | 3 月 | Qt 渲染性能瓶颈 |
| **C 替换 Qt** | 切换到 Slint / wxWidgets / Dear ImGui 之一 | 12 月 | Qt 商业政策变化或大版本破坏 |
| **D 自研** | 完全自研 GUI（基于 OpenGL） | 40 月+ | 不推荐 |

#### Plan B：替换为 Slint

[Slint](https://slint-ui.com/) 是个潜在的 Qt 替代候选：
- LGPL/GPL/商业三协议
- C++/Rust 双语言
- 声明式 UI（QML 风格）
- 跨平台

迁移策略：
- UI 描述用 `.slint` 文件 → 与 Qt `.ui` 文件对应
- C++ 业务逻辑通过 Slint 的 callback 接入
- 渐进迁移：新窗口用 Slint，老窗口保留 Qt，过渡期 6-12 月

#### 当前应做的预防性工作

- [ ] UI 层不持有业务状态 — 全部通过 CommandBus / Read Model 交互（已设计）
- [ ] Qt 信号槽不直接挂到 Domain 对象 — 必须经 EventConsumer 中转（已设计）
- [ ] 自定义 Widget 尽量用 OpenGL 直绘 — 减少 Qt 强依赖
- [ ] `.ui` 文件保持简单（让未来翻译到 .slint 更容易）

### 9.2.3 EnTT → 自研 ECS

**当前状态**：通过 `IEntityRegistry` 接口完全抽象，EnTT 类型只在 `EnttRegistry.cpp` 出现。

**自研估时**：中（**3-6 人月**） — ECS 设计模式成熟，参考资料多

**最现实的自研替代** — 一旦 EnTT 出问题，自研是合理选项。

#### 替代路径

| Phase | 目标 | 估时 | 触发条件 |
|---|---|---|---|
| **A 现状** | EnTT 3.x | — | 默认 |
| **B 自研最小版** | 实现 sparse set + view + observer | 3 月 | EnTT 维护断档 / 严重 bug |
| **C 自研增强版** | 加 group / signal / dispatcher 等高级特性 | 6 月 | 自研最小版上线后按需 |

#### 自研 ECS 的核心设计（已在 [05-code-skeletons §5.6](./05-code-skeletons.md) 设计接口）

```
mycad::infrastructure::ecs::MyCadRegistry
├── SparseSet<Component>     # 核心数据结构
├── EntityPool               # ID 复用
├── ViewBuilder              # 类型化 view
├── ObserverManager          # 订阅
└── Pool 自定义分配器           # 性能调优
```

参考实现可学：
- EnTT 源码（思路）
- bevy_ecs 设计文档
- "Entity Component System FAQ" by Sander Mertens

#### 当前应做的预防性工作

- [x] `IEntityRegistry` 已抽象（接口在 `src/domain/shared/include/mycad/domain/IEntityRegistry.hpp`）
- [x] Component 类型用 `concepts` 约束 → 替换实现时编译期保证兼容
- [ ] CI 加入"EnTT 类型不出 Adapter"检查（grep 规则）

### 9.2.4 PlaneGCS → 自研约束求解器

**当前状态**：通过 `IConstraintSolver` 接口抽象。集成 PlaneGCS 作为初始实现。

**自研估时**：中-高（**6-12 人月**） — 数值优化深水区

#### 替代路径

| Phase | 目标 | 估时 | 触发条件 |
|---|---|---|---|
| **A PlaneGCS** | FreeCAD 提取 | — | 默认 |
| **B Eigen LM 自研** | 基于 Eigen 的 Levenberg-Marquardt + 几何约束方程 | 6 月 | PlaneGCS 集成失败或性能不足 |
| **C 高级求解器** | 自适应 + 增量 + 并行 | 12 月 | 处理 1000+ 变量的大型草图 |

#### 自研基础架构

参考论文：
- Hoffmann, "A Survey of Geometric Constraint Solving Techniques"
- Bouma 等, "A Geometric Constraint Solver"

实现路径：
1. 用 Eigen 实现稀疏雅可比 + LM 求解
2. 从最简单约束（重合、距离）开始，逐步加入复杂约束
3. 用"约束分解 + 子系统求解"避免大矩阵
4. 增量求解通过保留上次的 LU 分解

#### 当前应做的预防性工作

- [x] `IConstraintSolver` 接口已设计
- [ ] Phase 1 建立"50 个典型草图基准集"用于回归测试
- [ ] Phase 0 阅读 Hoffmann 论文 + 自存笔记

### 9.2.5 OpenGL → wgpu / 自研渲染层

**当前状态**：通过 `IRenderPort` 接口抽象。OpenGL 仅在 `OpenGLRenderAdapter` 内出现。

**自研估时**：高（**12-24 人月**）

**真正问题**：苹果已弃用 OpenGL，长期 macOS 必须切换。

#### 替代路径

| Phase | 目标 | 估时 | 触发条件 |
|---|---|---|---|
| **A OpenGL 4.5** | 当前 | — | 默认 |
| **B + wgpu Adapter** | 加 wgpu 后端，覆盖 macOS Metal / Web WASM | 6 月 | macOS 真不可用 / Web 端需求 |
| **C 替换主后端** | wgpu 成为默认 | 3 月 | wgpu 1.0 + 工具链成熟 |
| **D 自研渲染** | 完全自研（不实际） | 24 月+ | 无现实驱动 |

#### Plan B：引入 wgpu

[wgpu](https://wgpu.rs/) 是 WebGPU 的 Rust 实现，但有 C++ 绑定（[wgpu-native](https://github.com/gfx-rs/wgpu-native)）。

迁移策略：
- 增加 `WgpuRenderAdapter` 与 `OpenGLRenderAdapter` 并存
- macOS 用户默认走 wgpu（通过 Metal）
- Windows / Linux 用户继续 OpenGL
- 远期统一到 wgpu

#### 当前应做的预防性工作

- [x] `IRenderPort` 接口设计
- [ ] OpenGL 不直接 #include 到 ECS Component 中（用 `GpuResourceComponent` 抽象 buffer handle）
- [ ] Shader 写法兼容 GLSL 与 SPIR-V（用 [shaderc](https://github.com/google/shaderc) 编译时转换）

### 9.2.6 EventStore（SQLite）→ 自研存储引擎

**当前状态**：`IEventStore` 接口抽象，初期 SQLite 实现。

**自研估时**：中-高（**6-12 人月**） — 但**几乎确定永远不需要**

#### 替代路径

| Phase | 目标 | 估时 | 触发条件 |
|---|---|---|---|
| **A SQLite** | 默认 | — | 单文件、单用户 |
| **B PostgreSQL** | 服务端协同场景 | 1 月 | 协同上线 |
| **C 自研嵌入式存储** | 极少需要 | 12 月+ | SQLite 真正成为瓶颈（极少） |

实际更可能的演进：SQLite → PostgreSQL（协同）→ 添加 Redis 缓存（性能） — **不会自研存储引擎**。

#### 当前应做的预防性工作

- [x] `IEventStore` 接口已设计
- [ ] 不直接依赖 SQLite 特性（如 JSON 函数），确保事件 schema 在 Postgres 也能跑

### 9.2.7 FlatBuffers → 自研序列化

**当前状态**：FlatBuffers schema 定义事件格式。

**自研估时**：低-中（**1-3 人月**） — 但**没必要**

序列化是工具型库，自研没有商业价值。这一项明确**不预留自研路径**。如果 FlatBuffers 出问题，迁移到 Protocol Buffers 或 Cap'n Proto 即可（同类工具替换）。

### 9.2.8 其他工具依赖（不预留自研）

| 库 | 自研估时 | 自研价值 | 备选 |
|---|---|---|---|
| Eigen | 50+ 月（数值算法库） | 无 | 无替代 — 标准化使用 |
| spdlog | 1 月 | 无 | quill / glog |
| fmt | 6 月 | 无 | std::format（C++23+） |
| Catch2 | 1 月 | 无 | doctest / GoogleTest |
| nlohmann/json | 0.5 月 | 无 | rapidjson |
| SQLite | 极高 | 无 | DuckDB / 系统数据库 |

---

## 9.3 自研路径的"决策矩阵"

何时启动自研评估的决策框架：

| 触发信号强度 | 上游活跃度 | 推荐动作 |
|---|---|---|
| 弱（轻微抱怨） | 活跃 | 提 Issue / PR 帮助上游修 |
| 中（明显问题） | 活跃 | 在 Adapter 内 patch（vendoring） |
| 中 | 不活跃 | **写 ADR 评估 fork or 替换** |
| 强（阻塞） | 任意 | **写 ADR 评估自研可行性** |
| 极强（商业模式受威胁） | 任意 | **必须立刻启动替代方案研究** |

---

## 9.4 长期主权的指标

每年评估一次"长期主权指数"：

| 指标 | Phase 1 目标 | Phase 2 目标 | Phase 3 目标 |
|---|---|---|---|
| Tier A 依赖数量 | ≤ 6 | ≤ 6 | ≤ 5 |
| Adapter 测试覆盖率 | ≥ 70% | ≥ 80% | ≥ 90% |
| Tier A 依赖被 patch 处数 | ≤ 3 | ≤ 5 | ≤ 5 |
| 关键依赖被自研替代数 | 0 | 0-1 | 1-2（最现实是 EnTT） |
| 单一上游决定项目存亡的依赖数 | 1（OCCT） | 1 | 0（理想） |

**目标**：Phase 3 末，OCCT 应该成为"重要但不致命"的依赖（即使 OCCT 停更，myCad 仍能在 12 个月内通过 Plan B 自救）。

---

## 9.5 反模式（不能做的事）

### 反模式 1：抽象出"OCCT 镜像接口"

❌ `IGeometryPort` 设计成 OCCT API 的 1:1 包装（如 `BRepPrimAPI_MakeBox`）
✅ `IGeometryPort` 用业务语义（如 `makeBox(dx, dy, dz)`），让 Adapter 翻译

**原因**：镜像接口绑死 OCCT 设计哲学，自研时接口必须重写 → 上层全部受影响。

### 反模式 2：让 Tier B/C 库类型出现在 Domain 层

❌ Domain 类持有 `Eigen::MatrixXd` 字段
✅ Domain 用自定义 `Matrix` 类型，Eigen 只在算法实现内部使用

**例外**：Eigen 的 `Vector3d` / `Quaterniond` 这种"事实标准"小型值对象可以直接用（成本/收益不平衡）。

### 反模式 3：插件 SDK 暴露第三方类型

❌ 插件接口签名出现 `Qt::QString` / `entt::entity` / `TopoDS_Shape`
✅ 插件接口完全用 myCad 自定义类型

**原因**：第三方类型暴露给插件 = 插件作者锁死在你的依赖版本上 = 升级第三方等于破坏所有插件。

### 反模式 4：依赖 vendoring 到项目内但绕过 vcpkg 管理

❌ 把 EnTT 源码复制到 `third_party/entt/` 直接编译
✅ vcpkg 管理 EnTT，需要时 vendoring 仅用于"自研替代实现需要参考实现"的场景

**原因**：vendoring 让升级困难，且容易让自己懒得升级。

### 反模式 5：抽象层但只有一种实现

❌ 只为了"以后可能换"做接口，但永远不写 Mock 实现
✅ 接口必须有至少 2 个实现：真实 Adapter + Mock（用于测试）

**原因**：单实现的接口本质上就是"耦合伪装" — 真换实现时会发现接口设计完全不适配。

---

## 9.6 与商业模式的耦合

自研路径直接服务于商业模式：

| 商业场景 | 关键依赖隔离 |
|---|---|
| **私有化部署给军工/医疗** | 客户可能要求"无外网依赖" → 所有依赖必须可离线包装 |
| **闭源商业插件** | 插件不能 link OCCT/Qt 静态 → Adapter 必须确保动态边界 |
| **AI 模型私有化** | 嵌入式 Python + ONNX Runtime → 不锁死特定云端 LLM |
| **企业版定制** | 客户可能要求替换某些库（如禁用 SQLite，用 Oracle） → Port 抽象使之可能 |

**核心命题**：依赖隔离不是"以后可能用得上的洁癖"，而是**直接的商业能力**。

---

## 9.7 维护责任

| 工件 | 维护者 | 更新频率 |
|---|---|---|
| Tier 分级清单 | 维护者 | 引入新依赖时 |
| 各依赖的"早期信号"监测（停更、协议变化） | 维护者 | 每月一次扫描 |
| Adapter 测试覆盖率报告 | CI 自动 | 每次 PR |
| 长期主权指数 | 维护者 | 每年一次（Phase 末） |
| 本文档 | 维护者 | 重大依赖变化时 |

---

> **最后修订**：2026-05（首次创建，对应 ADR-0009）
