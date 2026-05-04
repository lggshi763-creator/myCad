# ADR-0009: 第三方依赖隔离与长期自研路线

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 是个人开发者主导的开源 + 商业混合项目，长期目标横跨 5-10 年（CAD → CAM → CAE → BIM）。

**短期现实**：必须严重依赖第三方库（OCCT、Qt、EnTT、PlaneGCS、FlatBuffers、OpenGL 等）。如果不依赖，单人不可能在合理时间内交付。

**长期风险**：依赖意味着失去主权 — 上游一旦停更、协议变化、商业转向、社区分裂，项目都可能陷入被动。CAD 历史上有不少这样的案例（Autodesk 收购 Maya 后封闭、SolidWorks 收紧 SDK 等）。

**核心矛盾**：起步必须依赖第三方，长期需要主权 — 这两个目标必须同时管理。

不决策的代价：
- 不规划 → 依赖直接渗入 Domain → 未来想自研时发现接口完全不适配 → 必须重写上层
- 已经发生过的反模式（在其他项目中）：抽象出"OCCT 镜像接口"，自研 BRep 后发现接口绑死 OCCT 设计哲学，被迫重做接口 + 重做所有上层

## Decision

我们决定采用**三层依赖政策**与**Port-Adapter 强制隔离**，并为每个 Tier A 依赖**预留自研路径文档与触发条件**。

具体做法：

1. **Tier A**（核心依赖）：必须有 Port 抽象（如 `IGeometryPort`），第三方类型零泄漏，并在 [§九 self-host-strategy](../architecture/09-self-host-strategy.md) 中维护自研路径
2. **Tier B**（工具依赖）：直接使用，无需抽象（如 Eigen / FlatBuffers / spdlog）
3. **Tier C**（实用依赖）：可 vendoring（如 nlohmann/json / tl::expected）

每个 Tier A 依赖必须满足"五层防火墙"：
1. 接口在 Domain 层
2. 实现在 Infrastructure 层（命名 `XxxAdapter`）
3. 第三方类型隔离
4. CMake 依赖隔离（Domain target 不允许 link 第三方库）
5. 测试用 Mock Adapter

## Considered Alternatives

### Option A: 三层依赖政策 + 强制 Port 隔离
- 当前推荐
- 优点：长期主权可控，自研启动时上层零感知
- 缺点：早期开发抽象成本（每个核心依赖多写一层接口）
- 适用：长期项目 + 商业模式对依赖独立性敏感

### Option B: 完全拥抱第三方，无抽象
- 直接 #include OCCT / Qt 类型到任何层
- 优点：早期开发速度更快（少写接口层）
- 缺点：长期主权零；任何替换都是大手术
- 适用：短期项目 / 一次性原型

### Option C: 部分抽象（仅核心几何）
- 只为 OCCT 做抽象，Qt / EnTT 等直接用
- 优点：抽象成本中等
- 缺点：策略不一致，未来扩展时边界模糊

### Option D: 全自研
- 不依赖任何第三方
- 优点：终极主权
- 缺点：单人不可能，必死

## Rationale

**为什么不是 Option B（直接拥抱）**：
- myCad 商业模式之一是私有化部署给军工/医疗 — 这些客户对依赖独立性敏感
- 5-10 年项目跨度内，主流第三方库出问题的概率不低（Qt 已经历过 Nokia / Trolltech / Qt Company 多次易主）
- 早期"省下"的抽象成本，长期会以 10x-100x 倍数偿还

**为什么不是 Option C（部分抽象）**：
- 策略不一致 → 难以训练 AI 协作工具识别"这个依赖该不该 wrap"
- 容易漂移成 Option B
- 维护成本反而更高（每次新增依赖都要重新讨论分级）

**为什么不是 Option D（全自研）**：
- 单人开发者完全不现实
- CAD 行业 + 开源圈，自己造轮子等于"我不要 1000 万行已被验证的代码，我要 10 万行没人测过的我自己写的代码"

**Option A 的关键观察**：
- 抽象成本是**前置但有限**（每个 Tier A 多写一个接口 + Mock，约 1-3 天工作量）
- 自研触发概率低但不为零，触发时收益巨大（避免项目被绑架）
- 与 §三 AI 工作流配合：清晰的接口让 DeepSeek 实现 Adapter 时不会污染 Domain
- 与 §一商业模式配合：私有化部署、闭源插件、企业版定制都需要这种独立性

## Consequences

### Positive
- 长期主权可控：任何 Tier A 依赖出问题，可在 ADR 评估后启动自研
- 商业能力提升：私有化部署、闭源插件、企业定制都依赖这种隔离
- 测试友好：Mock Adapter 让 Domain 单测不需要第三方运行时
- 架构清晰：分层职责明确，AI 协作工具易学
- 抽象前置成本可量化（每个 Tier A 1-3 天）

### Negative
- 接口成本：每个核心依赖多写一层（缓解：模板代码可由 Claude Code 一次性生成）
- 抽象设计风险：接口设计不当会反过来绑死 Adapter（缓解：[§九 §9.5 反模式](../architecture/09-self-host-strategy.md) 提供清单）
- 性能开销：虚函数调用 + 接口转换（缓解：基本可忽略 — 几何/约束求解的实际成本远高于调用层级）
- 学习曲线：贡献者需要理解"为什么 OCCT 不能直接用"

### Neutral
- 需要维护 [§九 self-host-strategy](../architecture/09-self-host-strategy.md) 文档
- 需要 CI 加入"Tier A 类型不出 Adapter"的 grep 检查
- 需要每年评估"长期主权指数"（Tier A 数量、Adapter 测试覆盖等）
- ADR-RFC 流程必须强化，新增依赖必须经过 ADR

## Implementation Notes

### Phase 0 必须落地的接口（[§五 code-skeletons](../architecture/05-code-skeletons.md) 已设计）

- `IGeometryPort`（OCCT 隔离）
- `IEntityRegistry`（EnTT 隔离）
- `IConstraintSolver`（PlaneGCS 隔离）
- `IEventStore`（SQLite 隔离）
- `IRenderPort`（OpenGL 隔离）

### CI 强制约束（Phase 0 配置）

```yaml
- name: Verify Tier A type isolation
  run: |
    # Domain 层不能 #include OCCT / Qt / OpenGL / EnTT / SQLite
    bad=$(grep -r "#include" --include="*.hpp" src/domain/ | \
          grep -E "(OpenCASCADE|TopoDS|BRep|Geom_|gp_|QObject|Qt|GL/|gl3w|entt/|sqlite3)")
    if [ -n "$bad" ]; then
      echo "Tier A type leaked to Domain layer:"
      echo "$bad"
      exit 1
    fi
```

### 长期主权指数（每年 Phase 末评估）

| 指标 | Phase 1 | Phase 2 | Phase 3 |
|---|---|---|---|
| Tier A 依赖数量 | ≤ 6 | ≤ 6 | ≤ 5 |
| Adapter 测试覆盖率 | ≥ 70% | ≥ 80% | ≥ 90% |
| 单一上游决定项目存亡的依赖数 | 1 | 1 | 0（理想） |
| 关键依赖被自研替代数 | 0 | 0-1 | 1-2 |

### 自研触发评估流程

| 信号强度 | 上游活跃度 | 推荐动作 |
|---|---|---|
| 弱 | 活跃 | 提 Issue / PR |
| 中 | 活跃 | Adapter 内 patch |
| 中 | 不活跃 | 写 ADR 评估 fork or 替换 |
| 强 | 任意 | 写 ADR 评估自研 |
| 极强 | 任意 | 立刻启动替代研究 |

### 各 Tier A 依赖的自研估时（详见 [§九](../architecture/09-self-host-strategy.md)）

| 依赖 | 自研估时 | 现实可能性 |
|---|---|---|
| OCCT | 50-100 月 | 极低（除非项目变成全职 + 多人团队） |
| Qt | 40-80 月 / 替换 Slint：12 月 | 低（更可能替换非自研） |
| EnTT | 3-6 月 | **中（最可能真发生）** |
| PlaneGCS | 6-12 月 | 中 |
| OpenGL | + wgpu Adapter：6 月 | 中（macOS 趋势驱动） |
| SQLite | 6-12 月 | 极低 |

## References

- 详细策略：[§九 第三方依赖与自研路径](../architecture/09-self-host-strategy.md)
- Port 接口设计：[§五 code-skeletons](../architecture/05-code-skeletons.md)
- 决策清单：[§四 4.1.3 核心库](../architecture/04-tech-decisions.md)
- 商业模式关联：[§一 1.3 商业模式](../architecture/01-business.md)
- 风险关联：[§七 7.2 LGPL 影响](../architecture/07-risks.md)
