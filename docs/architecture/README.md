# 架构文档（docs/architecture/）

本目录存放 myCad 完整架构方案，按主题切分为 9 个独立 Markdown 文件，便于按章节维护与查阅。

回到顶层导航：[../../ARCHITECTURE.md](../../ARCHITECTURE.md)

## 文件清单

| 文件 | 主题 | 估读 |
|---|---|---|
| [01-business.md](./01-business.md) | 业务架构（定位、MVP、商业、生态） | 30 min |
| [02-technical.md](./02-technical.md) | 技术架构（四范式融合、DDD、事件溯源、ECS、插件、OCCT、渲染、求解器、文件） | 60 min |
| [03-ai-workflow.md](./03-ai-workflow.md) | AI 辅助开发工作流（Claude / DeepSeek / 人工） | 30 min |
| [04-tech-decisions.md](./04-tech-decisions.md) | 关键技术决策清单（含 IDE = Visual Studio 与自研路径列） | 20 min |
| [05-code-skeletons.md](./05-code-skeletons.md) | 7 个核心 C++20 头文件骨架 | 40 min |
| [06-roadmap.md](./06-roadmap.md) | Phase 0 → Phase 3 路线图（sprint 级任务） | 30 min |
| [07-risks.md](./07-risks.md) | 6 大风险识别与缓解 | 20 min |
| [08-vs-toolchain.md](./08-vs-toolchain.md) | **Visual Studio 2022 完整工具链** | 30 min |
| [09-self-host-strategy.md](./09-self-host-strategy.md) | **第三方依赖隔离 + 长期自研路径** | 30 min |

## 维护原则

- **小步频提**：架构调整应分多个小 PR 提交，每个 PR 触动文件不超过 2 个
- **同步更新**：代码与文档不一致 = bug，CI 中加入文档同步检查（远期）
- **ADR 配套**：任何"反转既有决策"或"引入新依赖"的修改必须配套 ADR（见 [../adr/](../adr/)）
- **可追溯**：每章末尾记录"最后修订日期 + 修订原因"

## 写作风格

- **中文为主，技术术语保留英文原文**（如 Aggregate / Bounded Context / Event Sourcing）
- **决策必须说"为什么"**：仅列"是什么"是反模式 — Claude/DeepSeek 后续无法基于"是什么"做合理推断
- **代码示例用 C++20 现代惯用法**：concepts、ranges、`std::expected`、RAII 严格
- **图优于文字**：能用 ASCII 流程图说明的，不要堆叠段落
