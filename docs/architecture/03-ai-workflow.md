# §三 AI 辅助开发工作流设计

> 回到导航：[../../ARCHITECTURE.md](../../ARCHITECTURE.md)

> 本章描述 myCad 的 AI 辅助开发工作流。所有工具配置以 **Visual Studio 2022** 为主 IDE — 详见 [§八 vs-toolchain](./08-vs-toolchain.md)。

## 3.1 工具分工矩阵

### 3.1.1 设计哲学：能力互补 vs 角色分工

**Claude Code 的本质优势**：
- 长上下文窗口（百万级 token），能看到完整架构文件 + 多个相关源文件
- 跨文件推理能力强，能识别架构违例
- 工具调用稳定（Read / Edit / Bash / Grep），适合"做计划 + 审查"
- 但**单次输出代码量受限**，且高频调用成本高

**DeepSeek 的本质优势**：
- 编码任务上的性价比极高
- 单次可输出大量代码（无 token 焦虑）
- 中文文档与注释生成自然
- 但**上下文窗口较小**，跨文件推理弱，需要喂"裁剪好的上下文"

**GitHub Copilot 的本质优势**（Visual Studio 内置）：
- 行内补全（输入时即时建议）
- 与 IDE 当前文件上下文紧密结合
- 写样板代码加速 30-50%
- 但**不适合大块新代码**或跨文件设计

**人工开发者（你自己）的不可替代性**：
- 业务决策（"这个功能值不值得做"）
- 跨架构层的设计权衡
- 生产环境的最终责任
- 与社区/客户沟通

### 3.1.2 完整任务分工表

| 任务类型 | 主要工具 | 辅助工具 | 人工干预点 |
|---|---|---|---|
| **架构设计** | Claude Code | — | 最终决策 |
| **接口设计（IPlugin / IGeometryPort 等）** | Claude Code | — | 评审签字 |
| **新增 DDD 聚合骨架** | Claude Code | — | 评审 |
| **新增领域事件类型** | Claude Code | — | — |
| **CommandHandler 编写** | Claude Code | DeepSeek（具体逻辑） | 复杂业务规则 |
| **OCCT 集成代码** | Claude Code | DeepSeek（API 包装） | OCCT API 选择 |
| **算法实现（约束求解、几何运算）** | DeepSeek | Claude Code（接口约定） | 算法选型 |
| **单元测试编写** | DeepSeek | Claude Code（覆盖率审查） | 边界用例确认 |
| **样板代码（CRUD、序列化、getter/setter）** | DeepSeek + Copilot | — | — |
| **GUI 代码（Qt 界面）** | DeepSeek + Copilot | Claude Code（架构合规审查） | UX 决策 |
| **重构** | Claude Code | DeepSeek（机械替换） | 重构边界确认 |
| **架构合规审查** | Claude Code | — | — |
| **性能分析** | Claude Code | — | 优化方案选择 |
| **Bug 调试** | Claude Code | DeepSeek（局部修复） + Copilot（行内辅助） | 复现验证 |
| **README / 用户文档** | Claude Code | DeepSeek（详细展开） | 内容审定 |
| **API 参考文档（Doxygen 注释）** | DeepSeek + Copilot | — | — |
| **博客文章 / 技术分享** | Claude Code（结构 + 论点） | DeepSeek（润色） | 最终发布 |
| **Git commit 信息** | Claude Code 或 Copilot | — | — |
| **PR 描述与审查回复** | Claude Code | — | 最终发送 |
| **Issue triage / 标签分类** | DeepSeek | — | 优先级决策 |
| **Code review（外部 PR）** | Claude Code | — | 合并决策 |

### 3.1.3 协同节点（多工具的关键交接点）

```
           ┌──────────────────────────────────┐
           │  人工：定义需求（"我要做 X 功能"）  │
           └──────────────┬───────────────────┘
                          ▼
           ┌──────────────────────────────────┐
           │  Claude Code：                    │
           │   - 读 ARCHITECTURE.md            │
           │   - 读相关现有代码                 │
           │   - 设计接口                      │
           │   - 输出 TASKS.md 任务卡           │
           │   - 输出 CONTEXT.md 切片           │
           └──────────────┬───────────────────┘
                          │ 交接：CONTEXT.md + TASKS.md
                          ▼
           ┌──────────────────────────────────┐
           │  DeepSeek：                      │
           │   - 读 CONTEXT.md（裁剪上下文）    │
           │   - 读任务卡                      │
           │   - 实现代码 + 单测                │
           │   - 输出 PR diff                  │
           │   （Copilot 在 Visual Studio 内辅助行内补全） │
           └──────────────┬───────────────────┘
                          │ 交接：diff + 测试结果
                          ▼
           ┌──────────────────────────────────┐
           │  Claude Code：                    │
           │   - 架构合规审查                   │
           │   - 检查接口契约一致性             │
           │   - 检查命名 / 风格               │
           │   - 输出审查报告                   │
           └──────────────┬───────────────────┘
                          ▼
           ┌──────────────────────────────────┐
           │  人工：审查报告决策                │
           │  - Approve → 合并                 │
           │  - Request Changes → 回到 DeepSeek │
           │  - Reject → 回到 Claude 重设计    │
           └──────────────┬───────────────────┘
                          ▼
           ┌──────────────────────────────────┐
           │  Claude Code：                    │
           │   - 写 ADR（如有架构决策）         │
           │   - 更新 ARCHITECTURE.md（如需）   │
           │   - 写 commit message             │
           └──────────────────────────────────┘
```

**为什么这样切割**：
- **Claude Code 负责"什么"和"为什么"**：架构、接口、约束、审查
- **DeepSeek 负责"怎么做"**：实现、测试、样板
- **GitHub Copilot 负责"打字加速"**：行内补全（与 Visual Studio 紧密集成）
- **人工负责"做不做"和"对不对"**：决策、最终责任 — 不可外包

---

## 3.2 CONTEXT.md 规范设计

### 3.2.1 CONTEXT.md 的本质

CONTEXT.md = "Claude Code 给 DeepSeek 的简报"。它解决的核心矛盾是：

> DeepSeek 上下文窗口装不下整个 myCad 代码库 → 必须有人替它做"知识抽取"

Claude Code 读完所有相关文件后，把 DeepSeek 完成当前任务**所必需且仅必需**的信息写入 CONTEXT.md。

模板见 [docs/ai-context/CONTEXT-template.md](../ai-context/CONTEXT-template.md)。

### 3.2.2 标准结构

```markdown
# CONTEXT for Task: <任务名>

> 由 Claude Code 生成于 <日期 + commit SHA>
> 适用任务：[TASK-NNNN](./TASKS.md#task-nnnn)
> 阅读时间预算：DeepSeek 应在 5 分钟内读完

## 1. 任务概要（1 段话）

## 2. 你需要知道的接口
### 2.1 必须实现/修改的接口
### 2.2 你将调用的接口（已存在）
### 2.3 相关值对象

## 3. 架构约束（必须遵守）
- **【硬约束】** ...

## 4. 禁止事项（常见错误模式）

## 5. 代码风格约定

## 6. 验收标准

## 7. 测试期望

## 8. 参考实现（可借鉴的类似代码）

## 9. 不在本任务范围内（不要做）
```

### 3.2.3 关键设计原则

**原则 1：可裁剪而非全量**
- CONTEXT.md 是"为单个任务量身定制的简报"，不是项目文档
- 完成一个任务后即可删除（git 跟踪历史，需要时可回查）

**原则 2：放代码片段而非引用**
- ❌ "请参考 src/foo/bar.hpp 的 Foo 类"
- ✅ 直接把 Foo 类的关键签名贴进来

理由：DeepSeek 没有"打开文件"的工具能力（在 myCad 工作流中），即使有也增加延迟。

**原则 3：禁止事项要具体**
- ❌ "注意架构合规"
- ✅ "不要在 Domain 层 #include OCCT 头"

理由：DeepSeek 对项目特有约束没有先验知识，必须显式列出。

**原则 4：附"参考实现"段**
- LLM 模仿能力 > 推理能力
- 给一个相似的已有实现，质量提升一个量级

---

## 3.3 TASKS.md 任务卡规范

### 3.3.1 标准格式

模板见 [docs/ai-context/TASKS-template.md](../ai-context/TASKS-template.md)。完整字段：

```markdown
# TASKS

## task-NNNN: <任务名>

**状态**: pending | in_progress | review | done | abandoned
**指派**: Claude Code（设计） + DeepSeek（实现） + 人工（评审）
**估时**: <小时分解>
**优先级**: P0 | P1 | P2
**依赖**: task-NNNN, task-NNNN

### 任务描述
### 业务背景
### 接口契约
### 输入 / 输出
### 关键算法说明
### 验收标准
   #### 功能性
   #### 测试用例（必须覆盖）
   #### 非功能性
### 架构约束
### 上下文文件
### 工作切分（建议给 DeepSeek 的子任务）
### 完成后的归档
```

### 3.3.2 完整示例：实现 SketchConstraintSolver

参见 [docs/ai-context/TASKS-template.md](../ai-context/TASKS-template.md) 中的 `task-0042` 示例（已包含 PlaneGCS 集成的完整任务卡）。

### 3.3.3 任务卡的设计原则

**原则 1：每个任务卡 1-2 周可完成**
- 超过 2 周的任务必须拆分
- 太小的任务（<2h）合并

**原则 2：验收标准用"可运行的测试"表达**
- ❌ "求解器要快"
- ✅ "100 变量 + 200 约束求解 < 100ms"

**原则 3：架构约束分硬/软**
- 硬约束：违反 = 拒绝合并
- 软约束：违反 = 评审讨论

**原则 4：永远附"工作切分"建议**
- 帮助 DeepSeek 自我管理上下文（每次会话只做一个 SubTask）

---

## 3.4 日常开发循环 SOP

### 3.4.1 单功能完整执行步骤

```
┌─────────────────────────────────────────────────────────────────┐
│ Step 1: 需求识别（人工）                                          │
│   - 来源：Issue / 路线图 / Bug 报告                                │
│   - 输出：一行需求描述                                             │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ Step 2: 任务分解（Claude Code）                                   │
│   输入：需求 + ARCHITECTURE.md + 当前代码状态                      │
│   工作：识别影响的限界上下文 + 列出接口 + 评估架构影响              │
│   输出：TASKS.md 中追加 1-N 个任务卡                              │
│   人工干预：审阅任务拆分是否合理                                   │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ Step 3: 接口设计（Claude Code）                                   │
│   输入：当前任务卡                                                │
│   工作：设计 C++ 头文件骨架 + Doxygen 注释                        │
│   输出：新增/修改 .hpp 文件，commit                               │
│   人工干预：评审接口设计                                          │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ Step 4: 上下文打包（Claude Code）                                 │
│   输入：任务卡 + 设计好的接口                                     │
│   工作：抽取 DeepSeek 完成此任务所需的所有信息                     │
│   输出：docs/ai-context/CONTEXT-task-NNNN.md                     │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ Step 5: 实现编码（DeepSeek + Copilot 行内辅助）                   │
│   输入：CONTEXT-task-NNNN.md + 任务卡                            │
│   工作：实现接口 + 写单元测试 + 自跑测试通过                       │
│   输出：feat/task-NNNN-xxx 分支 + PR                             │
│   工具：Visual Studio 2022 调试 + Test Explorer 自动发现 Catch2    │
│   人工干预：本地编译运行验证                                      │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ Step 6: 架构合规审查（Claude Code）                               │
│   输入：PR diff                                                  │
│   工作：用 §3.6.3 的"架构合规审查模板"                             │
│   输出：审查报告（评论形式贴入 PR）                                │
│   人工干预：决策 Approve / Request Changes                       │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
                ┌──────────┴──────────┐
                ▼                     ▼
        [Request Changes]         [Approved]
                │                     │
                ▼                     ▼
   ┌──────────────────────┐  ┌──────────────────────────┐
   │ 回到 Step 5          │  │ Step 7: 合并              │
   │ DeepSeek 修改        │  │ - Squash merge to main    │
   └──────────────────────┘  │ - 删除 feat 分支          │
                             └──────────┬───────────────┘
                                        ▼
                          ┌──────────────────────────────┐
                          │ Step 8: 文档更新（Claude）    │
                          │ - 必要时写 ADR               │
                          │ - 更新 ARCHITECTURE.md       │
                          │ - 移动 CONTEXT 到 archived/  │
                          │ - 标记任务卡为 done           │
                          └──────────┬───────────────────┘
                                     ▼
                          ┌──────────────────────────────┐
                          │ Step 9: Release Note 累积     │
                          │ - 写一行 CHANGELOG entry     │
                          └──────────────────────────────┘
```

### 3.4.2 时间预算（典型中等任务）

| 步骤 | 主要工具 | 时间 | 是否阻塞 |
|---|---|---|---|
| 1. 需求识别 | 人工 | 5 min | 是 |
| 2. 任务分解 | Claude | 15 min | 否 |
| 3. 接口设计 | Claude | 30 min | 是 |
| 4. 上下文打包 | Claude | 10 min | 否 |
| 5. 实现编码 | DeepSeek + VS | 2-4 h | 否 |
| 6. 合规审查 | Claude | 15 min | 是 |
| 7. 合并 | 人工 | 5 min | 是 |
| 8. 文档更新 | Claude | 15 min | 否 |
| 9. Release Note | Claude | 5 min | 否 |
| **总计** | | **~4 h** | |

### 3.4.3 关键决策点（不能外包给 AI）

| 决策点 | 何时出现 | 由谁决定 |
|---|---|---|
| 这个功能值不值得做 | Step 1 | 人工 |
| 任务拆分是否合理 | Step 2 | 人工评审 |
| 接口设计是否符合长期规划 | Step 3 | 人工评审 |
| 是否需要写 ADR | Step 6 后 | Claude 建议 + 人工决定 |
| Approve / Reject PR | Step 6 后 | 人工 |
| 何时发布 release | 累积 N 个任务后 | 人工 |

---

## 3.5 ADR（架构决策记录）规范

### 3.5.1 ADR 模板

完整模板见 [docs/adr/ADR-template.md](../adr/ADR-template.md)。结构：

- Status / Date / Decider / Author
- Context（背景）
- Decision（决策）
- Considered Alternatives（候选方案）
- Rationale（理由）
- Consequences（Positive / Negative / Neutral）
- Implementation Notes（实施注记）
- References

### 3.5.2 完整示例

参见 [docs/adr/ADR-0007-entt-as-ecs.md](../adr/ADR-0007-entt-as-ecs.md)（"采用 EnTT 作为 ECS 框架"），是规范化 ADR 的范例。

### 3.5.3 ADR 自动化流程

**Claude Code 何时主动建议写 ADR**：
- 引入新的第三方依赖（库、框架、工具）
- 改变跨多模块的接口或协议
- 选择两个方案中的一个，且选择不可逆
- 修改文件格式或事件 schema
- 改变项目的协议、构建系统、CI 流程

**ADR 不需要写的情况**：
- 单文件内的实现细节
- bug 修复
- 性能优化（除非引入新算法或库）
- 文档变更

详见 [docs/adr/README.md](../adr/README.md)。

---

## 3.6 Claude Code 提示词模板库

### 3.6.1 模板：新增领域事件

```
TASK: 在 myCad 中新增一个领域事件类型 <EventName>

请执行以下步骤：
1. 阅读 src/domain/<bounded-context>/include/ 下的现有事件定义
2. 阅读 ARCHITECTURE.md §2.2 与 §2.3，确认命名约定与字段约束
3. 设计新事件的字段（名称、类型、注释）
4. 输出：
   a) 新事件类的 .hpp 定义（继承 DomainEvent）
   b) FlatBuffers schema 中对应的 table 定义（schema/events.fbs）
   c) 序列化与反序列化代码
   d) 在 EventTypeRegistry 中注册的代码
   e) 该事件的 Doxygen 注释
5. 输出对应的单元测试

约束：
- 事件名称用过去时（XxxAdded / XxxModified / XxxRemoved）
- 字段全部 const
- 不要包含可变指针
- 序列化必须可往返

不要做：
- 不要修改其他事件
- 不要触动 EventStore 实现
- 不要写 CommandHandler

最后输出"任务摘要"。
```

### 3.6.2 模板：新增插件接口

```
TASK: 为 myCad 设计一个新的插件能力接口 <ICapabilityName>

背景：现有 IPlugin 通过 asXxxProvider() 模式暴露多种能力。

请执行：
1. 阅读 src/plugin/include/IPlugin.hpp 与现有 I{Format,Command,Feature,Ui}*.hpp
2. 阅读 ARCHITECTURE.md §2.5 关于插件总线的设计
3. 设计 <ICapabilityName>:
   a) 类的纯虚接口
   b) 配套的支持类型
   c) IPlugin::as<CapabilityName>Provider() 方法添加
4. 设计宿主侧管理：
   a) IPluginHost 上需要的注册/查询方法
   b) 在 PluginRegistry 中如何收集所有插件的此能力
5. 输出"如何编写一个该接口的示例插件"的代码骨架
6. 列出 ABI 兼容性考虑

约束：
- 接口方法必须返回 std::expected
- 不要在接口中暴露 OCCT/Qt 类型
- 元数据方法必须 const

提示：参考 IFormatHandler 的设计风格。

最后输出 ADR 草稿。
```

### 3.6.3 模板：架构合规审查

```
TASK: 对以下 PR 进行架构合规审查

PR: <PR 链接 或 commit SHA 范围>

请执行下列检查并出具报告：

## 1. 分层依赖检查
- [ ] Domain 层是否引入了 OCCT / Qt / OpenGL / 任何具体技术依赖
- [ ] Application 层是否绕过 Domain 直接访问 Infrastructure
- [ ] Infrastructure 实现是否通过接口注入
执行命令：grep -r "#include" --include="*.hpp" src/domain/ | grep -E "(OCCT|Qt|GL|gl)"

## 2. 接口契约检查
- [ ] 所有可能失败的方法是否返回 std::expected
- [ ] Aggregate 的业务方法是否返回 events 而非直接修改状态
- [ ] 事件类是否所有字段 const
- [ ] public 接口是否有 Doxygen 注释

## 3. ECS 使用规范
- [ ] EnTT 类型是否仅出现在 src/infrastructure/ecs/ 内
- [ ] Component 是否纯数据结构（无业务逻辑方法）
- [ ] 是否避免 dynamic_cast / RTTI 滥用

## 4. 测试覆盖
- [ ] 新增 public 方法是否有对应单测
- [ ] 错误路径是否被测试覆盖
- [ ] 边界条件是否覆盖

## 5. 命名与风格
- [ ] 类名 PascalCase，方法 camelCase，成员 trailing_underscore_
- [ ] include 顺序：本类→项目内→第三方→STL
- [ ] 文件名与主类名一致

## 6. 性能红线
- [ ] 渲染热路径（RenderSystem::update）是否有动态分配
- [ ] 是否在循环内重复创建 std::string / std::vector
- [ ] 是否误用 shared_ptr

## 7. 安全红线
- [ ] 是否有 raw new/delete
- [ ] 是否有未初始化的成员
- [ ] 是否有可能为 null 的指针被解引用而无 check

输出格式：
\`\`\`
### 🔴 阻塞性问题（必须修复才能合并）
### 🟡 非阻塞建议（建议修复）
### 🟢 优秀实践（值得肯定）
### 📊 总体评估
- 架构合规：✅ / ⚠️ / ❌
- 测试覆盖：估计 X%
- 是否建议合并：Approve / Request Changes / Reject
\`\`\`
```

### 3.6.4 模板：性能问题分析

```
TASK: 分析 myCad 中 <场景> 的性能瓶颈

场景：<具体描述>

请执行：

## 1. 收集现状数据
- 阅读用户提供的 Tracy / VS Profiler 数据
- 识别热函数（Top 10）
- 识别热分配点

## 2. 架构层面分析
- 阅读 src/<相关模块>/ 的实现
- 检查是否符合 ARCHITECTURE.md §2 中的性能预期
- 识别"违反架构却拖慢性能"的代码

## 3. 算法层面分析
- 数据结构选择是否合理（O(N²) → O(N log N)？）
- 是否有可缓存但被重复计算的内容
- 是否有可批处理但被逐一处理的工作

## 4. 系统层面分析
- 内存布局是否缓存友好（AoS vs SoA）
- 是否过度使用 shared_ptr 导致原子操作开销
- 是否有不必要的跨线程同步

## 5. 输出三类建议（按 ROI 排序）
- 🟢 立即可做（< 1 天，收益 > 30%）
- 🟡 中期改进（1 周内，收益 > 10%）
- 🔴 大手术（重构，谨慎决策）

## 6. 给出验证方案
- 如何编写 benchmark 测试这些假设
- 改进后预期的指标值

约束：
- 不要立即给代码改动；先输出分析与建议
- 任何"重构"建议必须配套 ADR 草稿
```

### 3.6.5 模板：代码重构

```
TASK: 重构 <文件/模块>

重构目标：<明确说明>

约束：
- 重构必须保持外部行为完全一致（所有现有测试继续通过）
- 单次 PR 只做一类重构
- 不引入新功能

请执行：
1. 阅读目标文件 + 所有调用方
2. 识别"职责块"
3. 设计新的文件/类划分
4. 输出重构方案：
   a) 新文件结构图
   b) 哪些代码移动到哪里
   c) 是否需要新增/修改接口
   d) 调用方需要的修改
5. 评估风险
6. 列出需要新增的测试以验证行为不变

不要做：
- 不要"顺便"修复你看到的 bug
- 不要改命名风格
- 不要做格式化

最后建议是否需要 ADR。
```

### 3.6.6 模板：Bug 调试

```
TASK: 诊断 myCad 中的以下 Bug

Bug 描述：<具体复现步骤 + 预期行为 + 实际行为>
环境：<OS, 编译器, myCad 版本, 触发文件>
工具：Visual Studio 调试器（断点 / 数据断点 / 时间旅行）

请执行：

## 1. 复现路径分析
- 阅读相关代码，画出"用户操作 → 触发 Bug 的代码路径"
- 标注每一步的关键状态

## 2. 假设生成
- 列出 3-5 个最可能的根因
- 按可能性排序

## 3. 验证策略
- 对每个假设给出"如何验证"的具体命令/代码
  （Visual Studio：在哪里下断点 / 数据断点的条件 / 看哪个变量）
  （或者：在哪里加 spdlog 输出）

## 4. 修复建议
- 找到根因后，最小化修复方案
- 如果根因暴露了架构问题，建议是否升级为 ADR 讨论

## 5. 防回归
- 写一个测试用例，覆盖此 Bug 的场景
- 评估是否有"同类 Bug"潜伏在其他类似代码中

约束：
- 不要"修一个症状放过根因"
- 不要做与 Bug 无关的"顺手清理"
- 修复 PR 必须附 regression test
```

---

## 3.7 工具链与 Visual Studio 的集成

详见 [§八 vs-toolchain](./08-vs-toolchain.md)。

关键集成点：
- **Test Explorer** 自动发现 Catch2 测试 → DeepSeek 实现完成后，VS 一键 run all tests
- **clang-tidy / clang-format** VS 内置 → DeepSeek 提交前自动 lint
- **CMakePresets.json** → 与 vcpkg 配合，DeepSeek 不需要手动配置依赖
- **GitHub Copilot** → DeepSeek 实现某段代码时，Copilot 给出行内补全
- **Tracy** → 性能分析时 VS 启动应用，Tracy server 实时观察

工具链不冲突，覆盖不同环节：
- 设计层（高维）：Claude Code
- 实现层（中维）：DeepSeek
- 编码层（低维）：Copilot 行内
- 调试层：VS 调试器 + Tracy
- 审查层：Claude Code

---

> **最后修订**：2026-05（首次拆分自 ARCHITECTURE.md，Visual Studio 集成已纳入）
