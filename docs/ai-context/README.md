# AI 协同上下文（docs/ai-context/）

本目录存放 myCad 项目的 **Claude Code + DeepSeek 协同模板与归档**。

回到架构总目录：[../../ARCHITECTURE.md](../../ARCHITECTURE.md)
工作流详细规范：[../architecture/03-ai-workflow.md](../architecture/03-ai-workflow.md)

## 本目录的作用

myCad 采用 AI 辅助开发工作流：
- **Claude Code**（架构 / 设计 / 审查 / 文档）
- **DeepSeek**（实现 / 单测 / 样板 / 算法）
- **GitHub Copilot**（Visual Studio 行内补全）
- **人工**（最终决策）

Claude Code 与 DeepSeek 之间通过两类标准化文件传递上下文：

| 文件类型 | 用途 | 模板 |
|---|---|---|
| **CONTEXT.md** | Claude Code 给 DeepSeek 的"任务简报"（每任务一份） | [CONTEXT-template.md](./CONTEXT-template.md) |
| **TASKS.md** | 任务卡（驱动整个工作流的核心文档） | [TASKS-template.md](./TASKS-template.md) |
| **Prompt Library** | 生成 Sprint 手册 / Phase 概览 / 任务卡 / 复盘 / 风险扫描 的 5 个可复制 prompt | [prompt-library.md](./prompt-library.md) |

## 目录结构

```
docs/ai-context/
├── README.md                       # 本文件
├── CONTEXT-template.md             # CONTEXT.md 模板
├── TASKS-template.md               # 任务卡模板（含完整示例 task-0042）
├── CONTEXT-task-NNNN.md            # 进行中的任务上下文（短期文件）
└── archived/                       # 已完成任务的归档
    ├── 2026-Q2/
    │   └── CONTEXT-task-0042.md
    └── ...
```

## 工作流概要

```
1. 人工 → 描述需求
2. Claude Code → 生成 TASKS.md 任务卡 + CONTEXT-task-NNNN.md
3. DeepSeek → 读 CONTEXT.md + 任务卡 → 实现代码 + 单测
4. Claude Code → 架构合规审查
5. 人工 → 决策 Approve / Request Changes
6. 合并 → CONTEXT 移动到 archived/
```

详见 [§三 §3.4 SOP](../architecture/03-ai-workflow.md)。

## 关键设计原则

### CONTEXT.md 原则

1. **可裁剪而非全量** — 为单任务量身定制，不是项目文档
2. **放代码片段而非引用** — DeepSeek 没有"打开文件"能力
3. **禁止事项要具体** — "不要在 Domain 层 #include OCCT 头" 而非"注意架构合规"
4. **附"参考实现"段** — LLM 模仿能力 > 推理能力

### TASKS.md 原则

1. **每任务 1-2 周可完成** — 超过必须拆分
2. **验收标准用"可运行的测试"表达** — "100 变量求解 < 100ms" 而非"求解器要快"
3. **架构约束分硬/软** — 硬约束违反 = 拒绝合并
4. **永远附"工作切分"建议** — 帮助 DeepSeek 自我管理上下文

## 归档管理

任务完成后：
1. 任务卡状态改为 `done`
2. 对应的 `CONTEXT-task-NNNN.md` 移动到 `archived/YYYY-Qn/`
3. 主 `TASKS.md` 中保留任务卡条目（不删除，作为历史参考）
4. 新任务从空白模板开始，不复用旧 CONTEXT

每季度归档可重新归类（按主题、按 Phase 等），便于历史回查。

## 维护责任

| 工件 | 维护者 | 更新频率 |
|---|---|---|
| CONTEXT-template.md | 维护者 | 工作流改进时 |
| TASKS-template.md | 维护者 | 字段调整时 |
| 进行中的 CONTEXT-task-NNNN.md | Claude Code 生成 + 维护者审定 | 任务期间 |
| archived/ | 自动 | 任务完成后 |
| 本 README | 维护者 | 工作流大改时 |
