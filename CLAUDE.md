# CLAUDE.md — myCad

> 项目级 AI 上下文。每次 Claude / Cursor / 任何 AI Code 助手启动 session 时自动加载。
> 仅描述**项目本身**与**不可违反的约束**。具体进度 / 待办 / 当前 sprint 状态等动态信息**不在**这里 —— 见各自专属位置（[devlog](docs/devlog/) / [inbox](docs/inbox.md) / [roadmap](docs/architecture/06-roadmap.md)）。
> 个人偏好（语言风格、AI 行为黑名单等）见用户级 `~/.claude/CLAUDE.md`。

---

## 1. 项目是什么

**myCad** — AI 原生的开源参数化 CAD 内核与桌面应用。LGPL-3.0 内核 + 商业插件生态 + 私有化部署友好。

**业务定位**（一句话）：在 FreeCAD 与 SolidWorks 之间补上一个真正现代的位置 —— 结构化历史、AI 原生设计循环、隐私敏感行业可自托管。

**架构范式**：DDD 六边形 × 事件溯源 + CQRS × ECS × 插件微内核（四范式融合）。

**技术栈**：

| 层 | 选型 | 决策 |
|---|---|---|
| 语言 | C++20 | — |
| 几何内核 | OpenCASCADE 7.x | [ADR-0004](docs/adr/ADR-0004-occt-as-geometry-kernel.md) |
| GUI | Qt 6 LTS（纯 CMake AUTOMOC，不依赖 Qt VS Tools） | [docs/architecture/08-vs-toolchain.md §8.2.5](docs/architecture/08-vs-toolchain.md) |
| 渲染 | OpenGL 4.5 | [ADR-0005](docs/adr/ADR-0005-opengl-not-vulkan.md) |
| ECS | EnTT 3.x | [ADR-0007](docs/adr/ADR-0007-entt-as-ecs.md) |
| 构建 | CMake 3.25+ + vcpkg manifest | [ADR-0008](docs/adr/ADR-0008-visual-studio-toolchain.md) |
| 测试 | Catch2 v3 + GoogleBenchmark | — |
| 协议 | LGPL-3.0-or-later | [ADR-0001](docs/adr/ADR-0001-license-lgpl-3.md) |

**目录拓扑**：

```
src/
├── domain/            ← 零外部依赖（ADR-0002）
│   ├── shared/        ← 跨子域的值对象 / 标识 / DomainEvent 基类
│   ├── drawing/       ← 工程图聚合（ADR-0010）
│   ├── sketch/        ← 草图聚合（Phase 1.A）
│   ├── feature/       ← 特征聚合（Phase 1.B）
│   └── assembly/      ← 装配聚合（Phase 2）
├── application/       ← CommandBus + handlers
├── infrastructure/    ← OCCT/EnTT/SQLite 等具体实现，实现 domain 的 port
├── plugin/            ← 插件 host + SDK（ADR-0006 双 ABI）
└── ui/                ← Qt6 widgets / viewport / gizmos
```

---

## 2. 不可违反的约束

### 2.1 架构边界（违反等于破坏 ADR）

| 规约 | 来源 |
|---|---|
| Domain 层零外部依赖（不许用 fmt / std::format / spdlog / Qt 任何头文件） | [ADR-0002](docs/adr/ADR-0002-domain-zero-deps.md) |
| 领域事件不可变 + 过去时命名（DomainEvent 一旦创建只读） | [ADR-0003](docs/adr/ADR-0003-events-immutable.md) |
| OCCT 仅由 `infrastructure/geometry/` 引用（domain / application 不许 `#include` OCCT 头） | [ADR-0004](docs/adr/ADR-0004-occt-as-geometry-kernel.md) |
| 插件双 ABI 边界（SDK 头不依赖 STL 异常 / 模板，C 风格入口） | [ADR-0006](docs/adr/ADR-0006-plugin-double-abi.md) |
| EnTT 实例仅由 `infrastructure/ecs/` 持有（不暴露给 application / domain） | [ADR-0007](docs/adr/ADR-0007-entt-as-ecs.md) |
| 工程图作为独立 domain 聚合，与 3D 模型通过 AggregateId 弱耦合 | [ADR-0010](docs/adr/ADR-0010-drawing-domain-boundary.md) |

### 2.2 文件 / include 路径

```
src/<layer>/<module>/include/mycad/<layer>/<X>.hpp     ← 公共头放这里
src/<layer>/<module>/<X>.cpp                           ← 实现放这里
```

include 形式**永远**带项目命名空间前缀：

```cpp
#include <mycad/domain/Hello.hpp>                     // ✅
#include <mycad/infrastructure/OcctGeometryAdapter.hpp>// ✅
#include "Hello.hpp"                                   // ❌ 无前缀
#include <shared/Hello.hpp>                            // ❌ 没有 mycad/ 命名空间
```

`target_include_directories` 只暴露 `<module>/include`，**不要**暴露 `<module>` 整个目录（避免封装破坏）。

### 2.3 Doxygen 注释规约

```cpp
/// @brief Builds a greeting string for the given name.
///
/// 给定名字生成形如 `Hello, <name>!` 的问候字符串。Unicode 安全。
///
/// @param  name  被问候者的名字。允许空、允许 UTF-8。
/// @return 拼好的问候字符串。
/// @throws std::bad_alloc 字符串内存分配失败时。
/// @thread-safe
std::string greet(std::string_view name);
```

| 维度 | 规则 |
|---|---|
| 注释符 | `///`（三斜杠）。**禁用** `/** ... */` |
| 标签风格 | `@brief` / `@param` / `@return` / `@throws` / `@see`。**不**用反斜杠形式 `\param` |
| brief | 一行英文，祈使句开头（Builds / Computes / Returns / Validates …） |
| 详情 | 中文为主，可混 markdown 代码块 |
| 自定义别名 | `@thread-safe` / `@noexcept-ok` / `@complexity{O(N)}` / `@si-units{millimeter}`（在 [Doxyfile](Doxyfile) 定义） |

样板文件（直接抄结构）：[src/domain/shared/include/mycad/domain/Hello.hpp](src/domain/shared/include/mycad/domain/Hello.hpp)。

### 2.4 CMake preset 红线

| 禁止 | 原因 |
|---|---|
| 在 `CMakePresets.json` / `CMakeUserPresets.json` **根对象**加任何注释字段（`$comment` / `_comment` / `$schema` 也不行） | CMake schema 拒绝 unknown field，整个文件无效化 |
| 在 `CMakeUserPresets.json` 写跟 `CMakePresets.json` **同名**的 preset | CMake 报 `Duplicate preset`。要覆盖必须改名 + `inherits` |
| 在不验证因果的情况下"为修构建错误而加 cache var" | 删掉它如果错误不复现，那就是 placebo 不是真修复 |

---

## 3. 知识地图（Where to look up X）

| 你需要了解 | 看这里 |
|---|---|
| 项目愿景 / 一句话定位 / 与 FreeCAD-SolidWorks 对比 | [README.md](README.md) |
| 架构总目录（七章 + ADR 索引） | [ARCHITECTURE.md](ARCHITECTURE.md) |
| **业务模型**：用户画像、商业模型、市场分析 | [docs/architecture/01-business.md](docs/architecture/01-business.md) |
| **技术架构**：四范式融合、层次图、聚合边界 | [docs/architecture/02-technical.md](docs/architecture/02-technical.md) |
| **AI 工作流**：Tier A/B/C 类型隔离、prompt 策略 | [docs/architecture/03-ai-workflow.md](docs/architecture/03-ai-workflow.md) |
| **技术决策汇总**（vs 各 ADR 单点） | [docs/architecture/04-tech-decisions.md](docs/architecture/04-tech-decisions.md) |
| **代码骨架**：每个 BC 的接口、命名约定、目录布局 | [docs/architecture/05-code-skeletons.md](docs/architecture/05-code-skeletons.md) |
| **路线图**（含 sprint 级任务、估时、验收） | [docs/architecture/06-roadmap.md](docs/architecture/06-roadmap.md) |
| **风险登记** | [docs/architecture/07-risks.md](docs/architecture/07-risks.md) |
| **Visual Studio 工具链**（preset / vcpkg / 调试 / natvis） | [docs/architecture/08-vs-toolchain.md](docs/architecture/08-vs-toolchain.md) |
| **长期自托管路线**（OCCT / Qt 替代论证） | [docs/architecture/09-self-host-strategy.md](docs/architecture/09-self-host-strategy.md) |
| **不可逆决策**（按编号查） | [docs/adr/](docs/adr/) — 索引在 [docs/adr/README.md](docs/adr/README.md) |
| **API 参考**（Doxygen + Sphinx 渲染） | `build/docs/sphinx/index.html`（生成后；`cmake --build --target docs`） |
| **风险评审 / 审计 / 复盘** | [docs/risk-reviews/](docs/risk-reviews/) / [docs/architecture-audits/](docs/architecture-audits/) / [docs/retrospectives/](docs/retrospectives/) |
| **AI 协作模板**（CONTEXT / TASKS / prompt-library） | [docs/ai-context/](docs/ai-context/) |
| **当前进度 / 周度小结** | [docs/devlog/](docs/devlog/) |
| **未分类的 followup**（跨 sprint 临时记事） | [docs/inbox.md](docs/inbox.md) |
| **未采纳的设计提案** | [docs/proposals/](docs/proposals/) |
| **可调用的 myCad 专用 skill** | `.claude/skills/`（vcpkg-rescue / adr-new / mycad-style-check） |

---

## 4. 写代码 / 改代码前的快速自检

读完上面任何一节后，开工前在脑子里过一遍这三个问题：

1. **要碰 domain 层吗？** 是 → 严格零外部依赖（ADR-0002）。哪怕 `#include <fmt/format.h>` 都不行。
2. **要新增公共 header 吗？** 是 → 路径必须是 `src/<layer>/<module>/include/mycad/<layer>/<X>.hpp`，include 形式必须 `<mycad/...>`。
3. **要新增第三方依赖吗？** 是 → **先**写 ADR 提案进 `docs/adr/`，**再**改 `vcpkg.json`。
