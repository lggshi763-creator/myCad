# TASKS

> 本文件是 myCad 项目所有任务卡的归集。每个任务卡是 Claude Code 与 DeepSeek 协同的核心驱动文档。
>
> 工作流详细规范：[../architecture/03-ai-workflow.md](../architecture/03-ai-workflow.md)

## 任务索引

| ID | 标题 | 状态 | Phase / Sprint | 优先级 | 估时 |
|---|---|---|---|---|---|
| task-0042 | 实现 SketchConstraintSolver | example | Phase 1 / Sprint 1.A2 | P0 | 8h |

> 此索引在新增任务时手动更新。

---

## 任务卡模板

```markdown
## task-NNNN: <任务名>

**状态**: pending | in_progress | review | done | abandoned
**指派**: Claude Code（设计） + DeepSeek（实现） + 人工（评审）
**估时**: <小时分解>
**优先级**: P0 | P1 | P2
**依赖**: task-NNNN, task-NNNN

---

### 任务描述

<2-3 段话描述任务做什么、做完后有什么效果>

### 业务背景

<为什么要做这个任务，对哪个 Phase / Feature 是关键>

### 接口契约

\`\`\`cpp
// 实现以下接口
class XxxHandler : public IXxx {
public:
    XxxHandler(...);
    std::expected<Result, Error> someMethod(...) override;
};
\`\`\`

### 输入

- ...

### 输出

- 成功：...
- 失败：...

### 关键算法说明

<如果涉及算法，简述关键步骤；否则省略>

### 验收标准

#### 功能性

1. ...
2. ...

#### 测试用例（必须覆盖）

\`\`\`cpp
TEST(XxxHandler, ScenarioA) { ... }
TEST(XxxHandler, ScenarioB) { ... }
TEST(XxxHandler, EdgeCaseC) { ... }
\`\`\`

#### 非功能性

- 单文件 < 500 行
- 所有 public 方法有 Doxygen 注释
- 编译警告 = 0
- clang-tidy 警告 = 0
- 性能：<具体指标>

### 架构约束

- **【硬约束】** ...
- **【软约束】** ...

### 上下文文件

详见 [docs/ai-context/CONTEXT-task-NNNN.md](./CONTEXT-task-NNNN.md)

### 工作切分（建议给 DeepSeek 的子任务）

1. SubTask A：...（Xh）
2. SubTask B：...（Xh）
3. SubTask C：...（Xh）

### 完成后的归档

- ✅ Code merged to main
- ✅ 必要时 ADR 写入
- ✅ ARCHITECTURE.md 引用此实现（如适用）
- ✅ CONTEXT-task-NNNN.md 移动到 archived/
- ✅ 任务卡状态改为 done
```

---

## 完整示例：task-0042

## task-0042: 实现 SketchConstraintSolver

**状态**: pending（示例任务卡，未实际启动）
**指派**: Claude Code（设计） + DeepSeek（实现） + 人工（评审）
**估时**: 8 小时（设计 1h + 实现 4h + 测试 2h + 审查 1h）
**优先级**: P0（阻塞 Phase 1 草图功能）
**依赖**: task-0038（PlaneGCS 已集成）, task-0040（Sketch 聚合根骨架已就位）

---

### 任务描述

实现 myCad 中的 2D 草图约束求解模块 `SketchConstraintSolver`，作为 `IConstraintSolver` 接口的具体实现，
内部委托给 PlaneGCS 库执行实际数值求解。

### 业务背景

用户在草图模式下添加几何约束（如重合、平行、距离）后，需要 myCad 求解出满足所有约束的几何位置。
这是 Sketch BC 的核心能力，没有它草图就只是"自由绘图"而非参数化设计。

属于 Phase 1 Sprint 1.A2 的关键任务，决定 MVP 草图功能能否交付。

### 接口契约

```cpp
// 实现以下接口
class SketchConstraintSolver : public IConstraintSolver {
public:
    SketchConstraintSolver(SolverConfig defaultConfig);

    std::expected<SolveResult, SolverError>
        solve(const SolveRequest& req) override;

    DofAnalysis analyze(const SolveRequest& req) override;

    std::expected<SolveResult, SolverError>
        incrementalSolve(SolverHandle handle,
                          std::span<const VariableUpdate> changes) override;
};
```

### 输入

- `SolveRequest`：包含变量列表 + 约束方程列表 + 初始猜值
- 变量来自草图实体（Point/Line/Circle 的几何参数）
- 约束类型：见 [§二 §2.8.2](../architecture/02-technical.md)

### 输出

- 成功：`SolveResult{status=Converged, solution=[...], iterations=N}`
- 失败：`std::unexpected(SolverError{kind=..., message=...})`

### 关键算法说明

PlaneGCS 内部使用 BFGS + DogLeg。我们只需要：
1. 把 SolveRequest 翻译为 PlaneGCS 的 GCS::System
2. 调 GCS::System::solve()
3. 把结果翻译回 SolveResult

### 验收标准

#### 功能性

1. 能求解"两点重合"约束（DOF=2 → 0）
2. 能求解"距离 100mm"约束（线段长度收敛到 100±1e-6）
3. 能求解 50 个随机几何 + 100 个约束的混合系统（< 100ms）
4. 能正确报告欠约束（DOF > 0 时返回 status=UnderConstrained）
5. 能正确报告过约束（冲突方程时返回 status=OverConstrained）
6. 增量求解比完整求解快至少 5x（在 100 变量场景下）

#### 测试用例（必须覆盖）

```cpp
TEST_CASE("SketchConstraintSolver resolves coincident constraint", "[solver]") { ... }
TEST_CASE("SketchConstraintSolver resolves distance constraint", "[solver]") { ... }
TEST_CASE("SketchConstraintSolver detects under-constrained system", "[solver]") { ... }
TEST_CASE("SketchConstraintSolver detects over-constrained system", "[solver]") { ... }
TEST_CASE("SketchConstraintSolver incremental solve is faster than full", "[solver]") { ... }
TEST_CASE("SketchConstraintSolver handles max iterations reached", "[solver]") { ... }
TEST_CASE("SketchConstraintSolver handles singular Jacobian", "[solver]") { ... }
```

#### 非功能性

- 单文件 < 500 行（如超出，分拆到 helper 文件）
- 所有 public 方法有 Doxygen 注释
- 编译警告 = 0
- clang-tidy 警告 = 0

### 架构约束

- **【硬约束】** 此实现位于 `src/infrastructure/solver/`，不是 `src/domain/`
- **【硬约束】** Domain 层只通过 `IConstraintSolver` 接口调用此实现（DIP）
- **【硬约束】** 不要在 `IConstraintSolver` 接口中泄漏任何 PlaneGCS 类型
- **【硬约束】** PlaneGCS 异常必须被捕获并转为 `std::expected`
- **【软约束】** 性能关键路径（incrementalSolve）禁止动态分配（用预分配缓冲）

### 上下文文件

详见 `docs/ai-context/CONTEXT-task-0042.md`（任务启动时由 Claude Code 生成）

### 工作切分（建议给 DeepSeek 的子任务）

1. **SubTask A**：写 `PlaneGCS::System` 与 `SolveRequest` 的双向翻译辅助函数（2h）
2. **SubTask B**：实现 `solve()` 方法 + 7 个单测（2h）
3. **SubTask C**：实现 `analyze()` + DOF 计算（1h）
4. **SubTask D**：实现 `incrementalSolve()` + benchmark（1h）

### 完成后的归档

- ✅ Code merged to main
- ✅ ADR-0017 写入（"为何选 PlaneGCS 而非自实现 Eigen LM" — 如尚未存在）
- ✅ ARCHITECTURE.md §2.8 引用此实现
- ✅ CONTEXT-task-0042.md 移动到 archived/2026-Q2/
- ✅ 任务卡状态改为 done

---

> 新任务请按上述模板复制并填充字段。模板中"task-0042"为示例，实际任务编号从 task-0001 起递增。
