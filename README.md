# myCad

> **AI 原生的开源参数化 CAD 内核与桌面应用**
>
> 让单人也能完成原本需要团队协作的机械设计任务。

[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)
[![Status: Phase 0 (Foundation)](https://img.shields.io/badge/status-Phase_0_Foundation-orange.svg)](./ROADMAP.md)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

---

## 这是什么

myCad 正在尝试解决三件事：

1. **结构化的 CAD 历史** — 用事件溯源（Event Sourcing）替代传统的"特征树重建" → 根除拓扑命名问题、原生支持协同编辑
2. **AI 原生的设计体验** — 自然语言到草图、设计意图捕捉、智能装配约束 — 不是"绑了一个聊天窗口"，而是设计循环里的一等公民
3. **私有化部署友好** — 隐私敏感行业（军工、医疗、国央企研发）能拿到一个可自托管、可深度定制、协议透明的工业级 CAD 选项

> 项目处于 **Phase 0（地基阶段）**，尚未发布。详见 [ROADMAP.md](./ROADMAP.md)。

## 一句话定位

myCad = LGPL 内核 + 商业插件生态 + AI 原生 + 私有化部署 = 在 FreeCAD 与 SolidWorks 之间补上一个真正现代的位置。

## 架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                      Presentation (Qt)                       │
└──────────────────────────────┬──────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                      Application (CommandBus)                │
└──────────────────────────────┬──────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                      Domain (DDD Aggregates, Events)         │  ★ 零外部依赖
└──────────────────────────────┬──────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────┐
│  Event Sourcing (EventStore + Snapshots)                    │
└──────────────────────────────┬──────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────┐
│  ECS Read Model (EnTT) + Render (OpenGL 4.5)                │
└─────────────────────────────────────────────────────────────┘

横向贯穿：Hexagonal Ports（IGeometryPort / IConstraintSolver / IPersistencePort / IPluginHost）
扩展机制：Microkernel Plugin Bus（C ABI + C++ Host 双层）
```

完整架构说明：[ARCHITECTURE.md](./ARCHITECTURE.md)

## 核心技术决策（速览）

| 维度 | 选型 | 备注 |
|---|---|---|
| 主语言 | C++20（Domain）+ Python（脚本插件） | [ADR-0002](./docs/adr/ADR-0002-domain-zero-deps.md) |
| 几何内核 | OpenCASCADE 7.7+ | [ADR-0004](./docs/adr/ADR-0004-occt-as-geometry-kernel.md) |
| GUI 框架 | Qt 6.5+ LTS | LGPL 动态链接 |
| ECS | EnTT 3.13+ | [ADR-0007](./docs/adr/ADR-0007-entt-as-ecs.md) |
| 渲染 | OpenGL 4.5 + DSA | [ADR-0005](./docs/adr/ADR-0005-opengl-not-vulkan.md) |
| 约束求解器 | PlaneGCS | [ADR-0009](./docs/adr/ADR-0009-self-host-roadmap.md) 列出未来自研路径 |
| 构建 | CMake 3.25+ + vcpkg manifest | |
| **IDE** | **Visual Studio 2022** | [ADR-0008](./docs/adr/ADR-0008-visual-studio-toolchain.md) |
| 协议 | LGPL-3.0-or-later | [ADR-0001](./docs/adr/ADR-0001-license-lgpl-3.md) |

完整决策清单：[docs/architecture/04-tech-decisions.md](./docs/architecture/04-tech-decisions.md)

## 快速开始（待 Phase 0 完成后填充）

```powershell
# 待 Phase 0.1 完成后填充
git clone https://github.com/<owner>/myCad.git
cd myCad
# Visual Studio 2022 推荐方式：
#   File → Open → CMake... → 选择 CMakeLists.txt
# 或命令行：
cmake --preset vs2022-debug
cmake --build --preset vs2022-debug
```

详见 [docs/architecture/08-vs-toolchain.md](./docs/architecture/08-vs-toolchain.md)。

## 本地生成 API 文档

API reference 通过 **Doxygen + Sphinx + Breathe** 三件套生成：Doxygen 扫源码产出
HTML 和 XML，Sphinx 拿 XML 渲染主站点。

### 一次性安装

| 工具 | Windows | Linux | macOS |
|---|---|---|---|
| Doxygen | `winget install doxygen` 或 [官网下载](https://www.doxygen.nl/download.html) | `apt install doxygen graphviz` | `brew install doxygen graphviz` |
| Graphviz（可选，类图用） | `winget install graphviz` | 同上 | 同上 |
| Sphinx + Breathe | `pip install sphinx breathe` | 同左 | 同左 |

> 建议用项目专用 venv：`python -m venv .venv && .venv\Scripts\activate && pip install sphinx breathe`

### 生成

**通过 CMake target（推荐）**：

```powershell
# 配置阶段会探测 Doxygen / Python / sphinx / breathe；齐全时启用 docs target
cmake --preset vs2022-x64-debug
cmake --build --preset vs2022-x64-debug --target docs
```

构建结束后打开：

```
build/docs/sphinx/index.html       ← Sphinx 主站点（含 Breathe 渲染的 API）
build/docs/doxygen/html/index.html ← Doxygen 原生 HTML（独立可读，含调用图）
```

**直接调用工具（跳过 CMake）**：

```powershell
# 1) 先生成 Doxygen XML
doxygen Doxyfile

# 2) 再让 Sphinx 消费它
python -m sphinx -b html docs/api-reference build/docs/sphinx
```

### 文档注释规约（速查）

```cpp
/// @brief Greets a person by name.
///
/// 给定名字生成形如 "Hello, <name>!" 的问候字符串。Unicode 安全，
/// 不会抛异常；空输入返回 "Hello, !"。
///
/// @param name 被问候者的名字（可为空、可含 Unicode）。
/// @return 拼好的问候字符串。
/// @see mycad::application::CommandBus
std::string greet(std::string_view name);
```

风格要点：

- 用 `///`（三斜杠），**不要**用 `/** ... */`。
- 结构化标签用 `@brief` / `@param` / `@return` / `@throws` / `@see`，**不**用反斜杠形式。
- **brief 一行用英文**，详细描述可中英文混排。
- 自定义别名（在 `Doxyfile` 中已定义）：`@thread-safe`、`@noexcept-ok`、`@complexity{O(N)}`、`@si-units{millimeter}`。

## 文档地图

| 文档 | 作用 |
|---|---|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | 架构总目录 |
| **[PLAYBOOK.md](./PLAYBOOK.md)** | **执行手册 — 从今天开始第一步做什么** |
| [docs/architecture/](./docs/architecture/) | 9 章完整架构方案（业务、技术、AI 工作流、决策、代码骨架、路线图、风险、VS 工具链、自研路径） |
| [docs/adr/](./docs/adr/) | 架构决策记录（ADR） |
| [docs/ai-context/](./docs/ai-context/) | Claude Code + DeepSeek 协同所用的 CONTEXT.md / TASKS.md 模板 |
| [ROADMAP.md](./ROADMAP.md) | Phase 0 → Phase 3 路线图 |
| [CONTRIBUTING.md](./CONTRIBUTING.md) | 贡献指南 |
| [CHANGELOG.md](./CHANGELOG.md) | 版本变更日志 |
| [LICENSE](./LICENSE) | LGPL-3.0-or-later |

## 协议

myCad 采用 **LGPL-3.0-or-later** 协议。详见 [LICENSE](./LICENSE) 与 [ADR-0001](./docs/adr/ADR-0001-license-lgpl-3.md)。

第三方依赖的协议清单与兼容性说明：[docs/architecture/04-tech-decisions.md §许可证矩阵](./docs/architecture/04-tech-decisions.md)。

## 鸣谢

myCad 站在以下开源项目的肩膀上：

- [OpenCASCADE](https://www.opencascade.com/) — 几何内核
- [Qt](https://www.qt.io/) — GUI 框架
- [EnTT](https://github.com/skypjack/entt) — ECS
- [PlaneGCS](https://github.com/FreeCAD/FreeCAD/tree/main/src/Mod/Sketcher/App/planegcs) — 约束求解器（FreeCAD 项目）
- [Eigen](https://eigen.tuxfamily.org/) — 线性代数
- [spdlog](https://github.com/gabime/spdlog) / [fmtlib](https://github.com/fmtlib/fmt) — 日志与格式化
- [FlatBuffers](https://google.github.io/flatbuffers/) — 序列化
- [Catch2](https://github.com/catchorg/Catch2) — 测试框架

完整鸣谢与"未来自研替代路径"：[docs/architecture/09-self-host-strategy.md](./docs/architecture/09-self-host-strategy.md)
