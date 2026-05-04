# myCad Prompt Library

> 复制即用的 AI 协作 prompt 库。每个 prompt 自包含、可粘贴给 Claude Code（或同等模型）执行。
>
> 使用建议：把每个 prompt 整段复制到 Claude Code 会话中，把 `<占位符>` 替换为实际值即可。

## 目录

| # | Prompt 名称 | 何时用 | 输出位置 |
|---|---|---|---|
| 1 | [生成 Sprint 详细手册](#1-生成-sprint-详细手册) | 每个 Sprint 启动时（每 2 周一次） | `docs/sprints/sprint-X.Y-playbook.md` |
| 2 | [生成 Phase 概览手册](#2-生成-phase-概览手册) | 进入新 Phase 时（约 3-12 月一次） | `docs/phases/phase-N-overview.md` |
| 3 | [生成任务的 CONTEXT.md + TASKS.md 条目](#3-生成任务的-contextmd--tasksmd-条目) | 每个具体任务开始时（高频） | `docs/ai-context/CONTEXT-task-NNNN.md` + 追加 `TASKS.md` |
| 4 | [生成 Sprint 复盘](#4-生成-sprint-复盘) | 每个 Sprint 末（每 2 周一次） | `docs/sprint-reviews/X.Y.md` |
| 5 | [生成月度风险扫描](#5-生成月度风险扫描) | 每月末（每月一次） | `docs/risk-reviews/YYYY-MM.md` |

> 配套阅读：[PLAYBOOK.md](../../PLAYBOOK.md)（执行手册整体哲学与 Sprint 0.1 完整样板）

---

## 1. 生成 Sprint 详细手册

**何时用**：每个 Sprint 启动日（每 2 周的周一），花 30-60 分钟生成本 Sprint 的"每日级"执行手册。

**输出**：`docs/sprints/sprint-X.Y-playbook.md`

**复制下方整段，替换 `<...>`**：

```
你是 myCad 项目的 Sprint 规划助手。

任务：为 Sprint <X.Y>（主题：<主题，例如"Domain 骨架"或"草图核心 — Sketch BC 聚合">）
生成"每日级"执行手册。

【请先阅读以下文件理解上下文】
1. PLAYBOOK.md — 整体执行哲学与 Sprint 0.1 的样板（必读）
2. docs/architecture/06-roadmap.md §<对应章节，例如 6.1 Phase 0 Sprint 0.2> — 本 Sprint 任务清单
3. docs/architecture/03-ai-workflow.md — AI 协作流程
4. docs/ai-context/CONTEXT-template.md + TASKS-template.md — 模板
5. docs/architecture/05-code-skeletons.md — 核心代码骨架（如本 Sprint 涉及 Domain 接口）
6. （如有）上一 Sprint 的复盘 docs/sprint-reviews/X.(Y-1).md
7. （如有）相关 ADR docs/adr/ADR-NNNN.md

【生成 docs/sprints/sprint-X.Y-playbook.md，结构如下】

# Sprint X.Y — <主题>（Week N-M）

## 阶段目标（一句话）

## Sprint 验收清单（端到端可验证）
- [ ] ...
- [ ] ...

## 任务总览
（从 06-roadmap.md 复制本 Sprint 表格，状态字段清空）

## 每日任务分解

### Day 1（约 2-3h）：<标题>
- 主用工具：🟦 Claude Code / 🟧 DeepSeek / 🟩 人工
- 前置：上一天必须完成什么
- 步骤：
  1. ...
  2. ...
- 给 AI 的 prompt（可复制粘贴）：
  ```
  <自包含 prompt，含文件路径 / 约束 / 输出格式>
  ```
- 验收（必须可勾选）：
  - [ ] ...
  - [ ] ...
- 卡点预案：如果 X 卡住 → ...

### Day 2 ~ Day N（同上结构）

## 本 Sprint 必须产出的 ADR（如有）
- ADR-XXXX: <标题> — 何时写 / 由谁起草

## Sprint 末验收清单
- [ ] 所有"任务总览"中任务状态为 done
- [ ] CI 三平台 green
- [ ] 新增/修改文件全部 commit
- [ ] dev log 当周完整
- [ ] （本 Sprint 主题相关的特定指标）

## 给下个 Sprint 的预热
- 下 Sprint 主题：...
- 启动前需要准备：...

【质量约束】
- 每天"实际编码时间" ≤ 3 小时（含 AI 等待）
- 每个 prompt 必须自包含（含文件路径与约束）
- 验收项必须可勾选（不要"持续优化"等模糊表述）
- 所有任务对应到 06-roadmap.md 中的具体行（不要凭空发明任务）
- 风险高的任务（OCCT 集成、约束求解器、Qt+OpenGL 集成）建议留 1-2 天 buffer
- 每个"给 AI 的 prompt"必须明确：要 AI 读哪些文件、输出格式、不要做什么

【不要做】
- 不要重复 PLAYBOOK.md §1 §4 §6 §7 已有内容（用引用即可）
- 不要超出本 Sprint 范围（即使下 Sprint 任务相关）
- 不要把"design + impl + test"塞进同一天（一天最多一类工作）
- 不要给"buffer 时间"占位符（明确写出"周日休息"或"buffer 用于消化超时"）

输出后请检查：
1. 总工时（实际编码时间累加）应在 16-25h 之间（双周 sprint 合理范围）
2. 每个 prompt 都能直接复制粘贴
3. 验收清单的项数 = 任务数 + 端到端剧本数
```

---

## 2. 生成 Phase 概览手册

**何时用**：进入新 Phase 时（Phase 0 → 1，约 3 个月后；Phase 1 → 2，约 12 个月后）。一次性生成整个 Phase 的"地图"。

**输出**：`docs/phases/phase-N-overview.md`

**复制下方整段，替换 `<...>`**：

```
你是 myCad 项目的 Phase 规划助手。

任务：为 Phase <N>（目标：<一句话总结，如 "MVP 草图建模" 或 "完整 CAD 工具">）
生成 Phase 级执行概览（不到日级，是 Sprint 与里程碑级）。

【请先阅读以下文件】
1. PLAYBOOK.md — 整体哲学
2. ROADMAP.md — 摘要级路线图
3. docs/architecture/06-roadmap.md §<本 Phase 章节>
4. docs/architecture/07-risks.md — 6 大风险
5. docs/architecture/01-business.md — 商业模式与里程碑
6. docs/architecture/04-tech-decisions.md — 已确定技术栈
7. （如有）上一 Phase 的 retrospective docs/retrospectives/phase-(N-1).md
8. 所有 docs/adr/*.md — 现有 ADR 全集

【生成 docs/phases/phase-N-overview.md，结构如下】

# Phase N — <主题>

## 一句话目标

## 验收剧本（端到端 N 步）
1. ...
2. ...
（必须可被外部用户验证 / 可被 CI 自动化）

## Sprint 列表

| Sprint | 周次 | 主题 | 关键交付 | 风险等级 | 状态 |
|---|---|---|---|---|---|
| X.1 | W1-W2 | ... | ... | 🟢/🟡/🔴 | pending |
| ... |

## 关键里程碑

| 里程碑 | 目标周次 | 二元判定标准 | 失败的回退方案 |
|---|---|---|---|

## 本 Phase 必须落地的 ADR

| ADR 编号 | 主题 | 何时写 | 起草者 |
|---|---|---|---|

## 本 Phase 主要 AI 协作场景

为本 Phase 列出 5-10 个高频场景，每个场景指明：
- 场景描述
- 推荐工具（Claude / DeepSeek / Copilot）
- 对应的 prompt-library.md 中哪个 prompt
- 预期产出形式

## 本 Phase 风险扫描

基于 docs/architecture/07-risks.md 的 6 大风险，针对本 Phase 给出：

| 风险 | 本 Phase 概率 | 本 Phase 影响 | 早期信号（本 Phase 特定） | 缓解动作 |
|---|---|---|---|---|

新增风险（如本 Phase 特有）：
- ...

## 商业进展节点

基于 docs/architecture/01-business.md，本 Phase 应达到：
- 用户数：...
- 第三方贡献者：...
- 商业关系：...

## 进入下个 Phase 的判据

必须满足以下全部条件才能进入 Phase <N+1>：
1. ✅ ...
2. ✅ ...
3. ✅ Phase <N> retrospective 已写

## 本 Phase 最容易失败的 3 件事 + 对策

1. **<失败模式 1>**
   - 触发条件：...
   - 早期信号：...
   - 对策：...
2. ...
3. ...

## 时间预算（含 ±50% buffer）

- 乐观：N 周
- 现实：N 周（推荐规划用此值）
- 悲观：N 周（启动 Plan B 阈值）

【质量约束】
- 必须基于已有路线图（不发明 Sprint）
- 时间预估保留 ±50% buffer
- 每个里程碑必须可二元判定（达到 / 没达到）
- 风险扫描必须给出"早期信号"（不只是"可能发生 X"）
- 商业节点必须可量化（数字 / 二元）

【不要做】
- 不要拆分到日级（那是 Sprint 手册的工作，用 prompt #1）
- 不要重复 ROADMAP.md（提供更深内容，否则免）
- 不要给乐观估时（个人开发者 50% 概率会偏 +50%）
```

---

## 3. 生成任务的 CONTEXT.md + TASKS.md 条目

**何时用**：每个具体编码任务启动前（高频，可能每天）。让 Claude Code 把"想法"翻译成 DeepSeek 能消化的"任务包"。

**输出**：
- `docs/ai-context/CONTEXT-task-NNNN.md`（新建）
- `TASKS.md`（追加条目，根目录或 `docs/ai-context/` 下，按项目约定）

**复制下方整段，替换 `<...>`**：

```
你是 myCad 项目的任务规划助手。

任务：为以下需求生成完整的 CONTEXT.md + TASKS.md 任务卡。

【需求描述】
<在这里描述任务，1-3 句>
例如：实现 Sketch 聚合根的 addLine 方法，遵循事件溯源模式

【元数据】
- 所属 Sprint：<Sprint X.Y>
- 预估时长：<X 小时>
- 优先级：<P0 / P1 / P2>
- 依赖任务：<task-NNNN, task-NNNN 或 无>

【请先阅读】
1. docs/ai-context/CONTEXT-template.md — 输出模板
2. docs/ai-context/TASKS-template.md — 任务卡模板
3. docs/architecture/02-technical.md §<相关 BC 章节>
4. docs/architecture/05-code-skeletons.md（如涉及核心接口）
5. 现有 TASKS.md（确定下一个任务编号；扫描类似任务避免重复）
6. 现有相关代码 src/<相关路径>/*.hpp src/<相关路径>/*.cpp（用 Read 工具）
7. 相关 ADR docs/adr/ADR-NNNN.md（如适用）

【输出 Part 1: docs/ai-context/CONTEXT-task-NNNN.md】

按 CONTEXT-template.md 11 个段落填充，特别注意：
- §2 接口必须粘贴完整代码片段（不能只写"参考 src/..."）
- §3 硬约束必须具体（"不要在 Domain 层 #include OCCT" 而非"注意架构合规"）
- §4 禁止事项必须基于本任务可能的错误模式（参考已有 ADR）
- §7 测试期望必须给出 Catch2 TEST_CASE 占位（DeepSeek 直接填充）
- §8 必须找到 1-2 个"参考实现"路径（已存在的相似代码）
- §9 "不在范围内"段必须明确防止 DeepSeek scope creep

【输出 Part 2: TASKS.md 追加条目】

按 TASKS-template.md 模板填充：
- 任务 ID 用下一个未使用编号（请扫描 TASKS.md 现有条目）
- 验收用例必须是可写成 Catch2 TEST_CASE 的具体场景
- 工作切分必须 ≤ 4 个 SubTask，每个 ≤ 4h
- 必须包含"完成后的归档"段（防止任务 done 后散落）

【质量约束】
- CONTEXT.md 阅读时间应 ≤ 5 分钟（DeepSeek 视角）
- 任务总时长 1-2 周内可完成（超出必须拆分为多个任务卡）
- 必须列出"不在范围内"段
- 接口契约必须严格遵守 ADR-0002（Domain 零依赖）/ ADR-0003（事件不可变）
- 涉及 Tier A 依赖（OCCT/Qt/EnTT/PlaneGCS/OpenGL/SQLite）的任务必须明确 Adapter 边界（参考 ADR-0009）

【不要做】
- 不要发明不存在的接口（如有需要先建议设计 PR，让我决定）
- 不要跳过 TASKS-template.md 中任何字段（用 "N/A" 占位也行）
- 不要把多个独立功能塞进一个任务（拒绝并要求拆分）
- 不要给 DeepSeek 不可执行的验收（如 "代码要优雅"）

输出格式：
- Part 1 用 ```markdown ... ``` 包裹
- Part 2 用 ```markdown ... ``` 包裹（注明追加到 TASKS.md 哪个位置）
- 末尾给一句"建议下一步"（例如"现在可以把 Part 1 + 任务卡发给 DeepSeek 开始实现"）
```

---

## 4. 生成 Sprint 复盘

**何时用**：每个 Sprint 末日（每 2 周的周日），30-45 分钟。

**输出**：`docs/sprint-reviews/X.Y.md`

**复制下方整段，替换 `<...>`**：

```
你是 myCad 项目的 Sprint 复盘助手。

任务：为 Sprint <X.Y>（<起始日 YYYY-MM-DD> ~ <结束日 YYYY-MM-DD>）生成复盘文档。

【请先阅读/收集】
1. docs/sprints/sprint-X.Y-playbook.md（计划）
2. docs/devlog/2026-WW*.md（实际记录，覆盖本 Sprint 期间）
3. 执行：git log --oneline --since="<起始日>" --until="<结束日>" —— 用 Bash 工具
4. CI 状态：用 Bash 跑 `gh run list --limit 50` 查本 Sprint failure 次数（需 gh CLI）
5. 本 Sprint 关闭的 issues / merged 的 PRs：`gh issue list --state closed --search "closed:>=<起始日>"`

【生成 docs/sprint-reviews/X.Y.md】

# Sprint X.Y 复盘 — <主题>

**周期**: <起始日> ~ <结束日>（<实际工作日数> 个工作日，<实际编码总时长> 小时）

## 计划 vs 实际

| 任务 | 计划估时 | 实际耗时 | 状态 | 偏差原因 |
|---|---|---|---|---|
（从 sprint-X.Y-playbook.md 任务总览复制 + 填充实际值）

## 关键产出

### 代码
- 新增行数：+X
- 删除行数：-X
- 净增模块：<列出>
- 测试覆盖率变化：<前 → 后>

### 文档
- 新增 ADR：ADR-NNNN <标题>
- 新增/修改架构文档章节：<列出>

### 工具链
- CI 改进：<是否新增 job>
- 新依赖：<vcpkg.json 改动>

## 卡点与解决方案

按时间顺序：

1. **<卡点 X>**
   - 出现日期：YYYY-MM-DD
   - 卡住时长：<小时>
   - 根因：...
   - 解决方案：...
   - 后续避免：...

2. ...

## 估时偏差分析

- 整体偏差：+X%（实际 vs 计划）
- 高偏差任务（> 100% 超出）：
  - <任务名>：原因 — 改进
- 低偏差任务（值得重复的方法）：
  - <任务名>：成功因素 — 提炼为模式

## 给下个 Sprint 的调整

- **删除**（不再做）：...
- **新增**（应该做没做）：...
- **调整估时**：<具体任务的新估时>
- **流程改进**：<工作流哪里出了问题，怎么改>

## 心理健康自检（参考 PLAYBOOK §8）

- 工作节奏：维持每天 ≤ 3h ? 周休 1 天 ?
- 红色信号检查（参考 §7.1.2 早期信号清单）：
  - 连续无 commit > 3 天：是 / 否
  - sprint 估时全面超出：是 / 否
  - "嫉妒团队"念头出现：是 / 否
- 整体感受：1-10 分（10 = 兴奋，1 = 想放弃）

## 长期主权指数（季度更新一次，平时仅检查变化）

- 本 Sprint 是否引入新依赖：<是 / 否>，详情：...
- 是否有 Tier A 类型泄漏（CI 自动检查）：是 / 否
- Adapter 测试覆盖率变化：<前 → 后>

## 待跟进项（Open Loops）

- [ ] <跨 Sprint 未完成的事，下个 Sprint 要处理>
- [ ] ...

## 公开传播素材（如有）

本 Sprint 哪些事可以写成博客 / B 站视频 / 社交媒体内容（参考 PLAYBOOK §8.1）：
- ...

【质量约束】
- 必须基于客观数据（git log / CI / commit 数 / dev log）
- 估时偏差必须分析根因（不只是"超了"）
- 给下 Sprint 的建议必须具体可执行
- 心理健康自检必须诚实（这是给自己的，不是给社区的）

【不要做】
- 不要写"总体不错，下次继续"的废话
- 不要回避 Sprint 失败的事实（失败的复盘价值最高）
- 不要美化数据
- 不要超出本 Sprint 范围（不要顺便复盘上 Sprint）
```

---

## 5. 生成月度风险扫描

**何时用**：每月最后一个周日（与最后一个 Sprint 复盘同期或之后）。

**输出**：`docs/risk-reviews/YYYY-MM.md`

**复制下方整段，替换 `<...>`**：

```
你是 myCad 项目的风险评估助手。

任务：为 <YYYY-MM> 月生成风险扫描报告。

【请先阅读】
1. docs/architecture/07-risks.md — 6 大风险全集与缓解措施
2. 本月所有 docs/sprint-reviews/*.md（通常 2 份）
3. 执行：git log --oneline --since="<月首日>" --until="<月末日>" -- 看 commit 趋势
4. （如有）上月风险扫描 docs/risk-reviews/YYYY-(MM-1).md 的"待跟进"项
5. 关键依赖近期动态（手动查或委托 WebSearch）：
   - OCCT 主仓近 3 个月 commit 频率
   - Qt 公司近期公告
   - PlaneGCS / EnTT 等近期变化

【生成 docs/risk-reviews/YYYY-MM.md】

# 风险扫描 - YYYY-MM

**评估日期**: YYYY-MM-DD
**评估人**: <你的名字>
**本月 commits**: N | **PR merged**: M | **Issues closed**: K

## 6 大风险评分变化

| 风险 | 上月 P/I | 本月 P/I | 变化方向 | 变化原因 |
|---|---|---|---|---|
| R1 资源瓶颈 | 🔴/🔴 | ?/? | ↑ / → / ↓ | ... |
| R2 LGPL 商业化 | 🟡/🟡 | ?/? | ... | ... |
| R3 求解器实现 | 🟡/🔴 | ?/? | ... | ... |
| R4 架构漂移 | 🔴/🟡 | ?/? | ... | ... |
| R5 生态冷启动 | 🟡/🔴 | ?/? | ... | ... |
| R6 大厂竞争 | 🟢/🟡 | ?/? | ... | ... |

P = 概率，I = 影响，🟢低 / 🟡中 / 🔴高

## 早期信号检测

对每个风险逐一检查 §七 中列出的"早期信号"清单：

### R1 资源瓶颈
- [ ] 连续 2 周以上没有 commit：触发 / 未触发
- [ ] Sprint 估时频繁 2x 超出：触发 / 未触发
- [ ] 把 P1 任务降级为 P2 自我安慰：触发 / 未触发
- [ ] 周末"只想休息"超过 3 个连续周末：触发 / 未触发
- [ ] "如果有团队多好"念头持续出现：触发 / 未触发
- [ ] 健康指标恶化（睡眠、运动、社交）：触发 / 未触发

判定：🟢 全未触发 / 🟡 1-2 项触发 / 🔴 3+ 项触发

### R2 ~ R6 同上结构

## 新发现的风险（不在 §七 6 项内）

| 编号 | 风险 | 概率 | 影响 | 早期信号 | 建议缓解 |
|---|---|---|---|---|---|
| R7 | <例：vcpkg baseline 大版本升级破坏依赖> | 🟢 | 🟡 | ... | ... |

## 缓解措施有效性评估

对上月已部署的缓解措施评估：

| 措施 | 部署月份 | 执行情况 | 效果 | 是否继续 |
|---|---|---|---|---|
| M1.1 严格时间盒 | YYYY-MM | 完全 / 部分 / 没执行 | ... | 是 / 调整 |

## 本月行动项

### 立即处理（本周内）
1. <具体行动>，截止 YYYY-MM-DD

### 下月跟进
1. <具体行动>，截止 YYYY-MM-DD

### 季度评估
1. <长期事项>

## 长期主权指数（每年 Phase 末更新一次，平时检查）

| 指标 | 上次值 | 本次值 | 趋势 |
|---|---|---|---|
| Tier A 依赖数量 | 6 | ? | → |
| Adapter 测试覆盖率 | 70% | ?% | ↑ / ↓ |
| Tier A 依赖被 patch 处数 | 0 | ? | → |
| 单一上游决定项目存亡的依赖数 | 1（OCCT） | ? | → |

## 总结（一段话）

<本月风险整体趋势：好转 / 稳定 / 恶化；最需要注意的 1-2 项；下月重点>

【质量约束】
- 必须基于客观证据（不要"我觉得 R5 风险变高"）
- 行动项必须有 owner（你自己）+ 截止日期
- 早期信号检查必须逐项过（不要"全部未触发"草率打勾）
- 对上月已采取措施必须诚实评估（包括"没执行"）

【不要做】
- 不要重复 §七 内容（提供监测数据 + 判断）
- 不要给 6 项都标"无变化"（认真检查必有变化）
- 不要避谈失败的缓解措施
- 不要把"风险变高"等同于"项目要完"（只是需要关注）
```

---

## 使用建议

### 时间分配建议

| Prompt | 频率 | 单次耗时 | 月累计 |
|---|---|---|---|
| #1 Sprint 手册 | 双周一次 | 30-60 min | ~2h |
| #2 Phase 概览 | 每 3-12 月 | 1-2h | 极少 |
| #3 任务卡 | 高频（每天可能多次） | 5-15 min | ~3-5h |
| #4 Sprint 复盘 | 双周一次 | 30-45 min | ~1.5h |
| #5 月度风险扫描 | 每月一次 | 45-60 min | ~1h |

**月度规划开销总计**：约 7-10 小时/月（占总编码时间 ~10-15%，是合理投入）。

### 与 PLAYBOOK 的关系

- **PLAYBOOK.md** = 工作哲学 + Sprint 0.1 完整样板 + 长期参考
- **prompt-library.md**（本文件）= 生成后续 Sprint / Phase 手册的工厂
- **生成的 sprint-X.Y-playbook.md** = 当前正在执行的具体行动指南

工作流：
```
Sprint 启动日 → prompt #1 → 生成 sprint-X.Y-playbook.md → 跟着执行
            ↓
Sprint 中每日 → prompt #3（按需）→ 生成任务卡 → 给 DeepSeek 实现
            ↓
Sprint 末日 → prompt #4 → 生成 sprint-X.Y review
            ↓
月末 → prompt #5 → 生成月度风险扫描
            ↓
Phase 末 → prompt #2 → 启动下个 Phase
```

### 提示词的演进

每用一次本库的 prompt，注意：
- 哪些信息 AI 必须再问一次（说明 prompt 不够自包含 → 改进）
- 哪些产出格式不对（说明输出格式描述不清 → 改进）
- 哪些质量约束被忽略（说明约束不够强制 → 改进）

把改进直接编辑到本文件。**这是一个活的文档**。

---

> **最后修订**：2026-05（首版）
