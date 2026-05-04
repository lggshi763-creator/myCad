# myCad 架构总览

本文件是架构方案的入口。完整方案分为 9 章，按主题拆分到 [docs/architecture/](./docs/architecture/) 下。

> 设计哲学：**DDD 六边形 × 事件溯源+CQRS × ECS × 微内核插件总线** 四范式融合，配合 **AI 原生开发工作流**（Claude Code 设计 + DeepSeek 实现）与**第三方依赖隔离 + 长期自研路径**。

## 章节导航

| # | 章节 | 文件 | 简介 |
|---|---|---|---|
| §一 | 业务架构 | [01-business.md](./docs/architecture/01-business.md) | 产品定位、MVP、商业模式、生态战略 |
| §二 | 技术架构 | [02-technical.md](./docs/architecture/02-technical.md) | 四范式融合、DDD 限界上下文、事件溯源、ECS、插件总线、OCCT 集成、渲染、求解器、文件格式 |
| §三 | AI 辅助开发工作流 | [03-ai-workflow.md](./docs/architecture/03-ai-workflow.md) | Claude/DeepSeek/人工分工、CONTEXT.md/TASKS.md/ADR 规范、提示词模板库 |
| §四 | 关键技术决策清单 | [04-tech-decisions.md](./docs/architecture/04-tech-decisions.md) | 全部技术选型 + 自研路径估时与触发条件 |
| §五 | 核心代码骨架 | [05-code-skeletons.md](./docs/architecture/05-code-skeletons.md) | 7 个关键模块的 C++20 头文件骨架 |
| §六 | 分阶段开发路线图 | [06-roadmap.md](./docs/architecture/06-roadmap.md) | Phase 0 → Phase 3 的 sprint 级任务清单 |
| §七 | 风险识别与缓解 | [07-risks.md](./docs/architecture/07-risks.md) | 6 大风险 × 概率/影响/缓解 |
| §八 | **Visual Studio 工具链** | [08-vs-toolchain.md](./docs/architecture/08-vs-toolchain.md) | VS 2022 + CMake + vcpkg 完整流程；调试、测试、插件开发、Python 集成 |
| §九 | **第三方依赖与自研路径** | [09-self-host-strategy.md](./docs/architecture/09-self-host-strategy.md) | 每个核心第三方依赖的隔离层、自研估时、迁移触发条件 |

## 阅读建议

| 角色 | 推荐阅读顺序 |
|---|---|
| **首次接触项目** | README.md → §一（30 min）→ §二（60 min）→ §六（30 min） |
| **第三方插件开发者** | README.md → §二.5（插件总线）→ §五.4（IPlugin）→ §三.2（CONTEXT 规范）|
| **企业评估者** | README.md → §一.3（商业模式）→ §九（自研路径，长期独立性论证）→ §七（风险）|
| **架构贡献者** | 全部 9 章 + [docs/adr/](./docs/adr/) 全部 ADR |
| **AI 协作开发者** | §三 全章 + [docs/ai-context/](./docs/ai-context/) 模板 |

## 与 ADR 的关系

架构文档描述"是什么 / 怎么做"，[ADR](./docs/adr/) 记录"为什么这样选 / 不那样选"。

每个重要决策都对应一个 ADR：

- 协议选型 → [ADR-0001](./docs/adr/ADR-0001-license-lgpl-3.md)
- Domain 零依赖 → [ADR-0002](./docs/adr/ADR-0002-domain-zero-deps.md)
- 事件不可变 → [ADR-0003](./docs/adr/ADR-0003-events-immutable.md)
- OCCT 几何内核 → [ADR-0004](./docs/adr/ADR-0004-occt-as-geometry-kernel.md)
- OpenGL 而非 Vulkan → [ADR-0005](./docs/adr/ADR-0005-opengl-not-vulkan.md)
- 插件双 ABI 边界 → [ADR-0006](./docs/adr/ADR-0006-plugin-double-abi.md)
- EnTT 作为 ECS → [ADR-0007](./docs/adr/ADR-0007-entt-as-ecs.md)
- **Visual Studio 工具链** → [ADR-0008](./docs/adr/ADR-0008-visual-studio-toolchain.md)
- **第三方依赖与自研路线** → [ADR-0009](./docs/adr/ADR-0009-self-host-roadmap.md)

## 文档维护规则

1. **架构变更 → 同步更新本目录与子文档**：任何接口变更或决策反转必须立即在文档反映
2. **新增重要决策 → 写 ADR**：标准见 [docs/adr/README.md](./docs/adr/README.md)
3. **每月架构体检** → 输出到 `docs/architecture-audits/YYYY-MM.md`（防止架构漂移）
4. **每季度风险复审** → 输出到 `docs/risk-reviews/YYYY-Qn.md`
5. **每个 Phase 末写 retrospective** → `docs/retrospectives/phase-N.md`

## 反馈

架构相关讨论请用 GitHub Discussions 的 `architecture` 分类；具体提案请走 ADR-RFC 流程（见 [docs/adr/README.md](./docs/adr/README.md)）。
