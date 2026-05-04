# ADR-0003: 领域事件不可变 + 过去时命名

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 采用事件溯源（Event Sourcing）作为核心架构。领域事件是事件溯源的"原子单位" — 所有业务真相都从事件流派生。

事件设计的细节（命名、可变性、字段约束）会深刻影响整个系统的可靠性：
- 事件是否可变 → 决定能否安全持久化、回放、审计
- 事件命名 → 影响开发者思维（"做什么" vs "记录什么发生了"）
- 事件字段约束 → 影响 schema 演进与序列化

不决策的代价：早期写 100 个事件后才发现"原来事件应该不可变"，重构成本巨大。

## Decision

我们决定所有领域事件遵循以下规则：

1. **事件命名为过去时**：`SketchEntityAdded`、`FeatureRebuilt`、`SketchSolved` —— 表达"已经发生的事实"，而非"将要做"。
2. **事件类的所有字段为 `const`**：构造时填充，永不修改。
3. **事件不可被 mutate**：没有 setter，没有"模拟反向操作"的方法。
4. **事件继承自 `DomainEvent` 基类**：用虚函数 + RTTI 替代实现，便于插件扩展自定义事件。
5. **事件类型名通过静态 `kTypeName` 字符串暴露**：用于序列化反序列化分发。
6. **每个事件必须可序列化**：通过 `IEventSerializer` 接口实现。

## Considered Alternatives

### Option A: 不可变 + 过去时命名（推荐）
- 优点：事件溯源标准做法；防止误改历史；线程安全
- 缺点：构造样板略多

### Option B: 事件可变（命令式）
- 类似传统 OO："SaveSketch" 而非 "SketchSaved"
- 优点：开发者熟悉
- 缺点：事件溯源 anti-pattern；线程不安全；序列化语义模糊

### Option C: 事件用 `std::variant` 而非继承
- 类型在编译期固定
- 优点：性能略好（无虚函数）
- 缺点：插件无法扩展事件类型；表达力弱

### Option D: 不区分"事件"与"命令"，统一用 message
- Akka / Erlang 风格
- 优点：消息传递哲学清晰
- 缺点：CQRS 变模糊；与 DDD 不契合

## Rationale

- **事件不可变是事件溯源的"第一公理"**：可变事件 = 历史可被篡改 = 审计、回放、协同全部失效
- **过去时命名强制思维转变**：CRUD 思维（"添加这个东西"）会阻碍事件溯源的设计；"添加这个东西的事实已经发生"是正确语义
- **基类 + 虚函数让插件可扩展**：CAM/CAE/BIM 插件需要定义自己的事件类型，`std::variant` 编译期固定无法扩展
- **静态 kTypeName 是反序列化分发的最简洁方案**：比 RTTI / 注册宏更轻量
- **`const` 字段**：编译期保证，C++ 圈最直接的不可变表达；零运行时成本

## Consequences

### Positive
- 事件溯源核心保证：历史不可被篡改
- 线程安全（不可变对象天然线程安全）
- 序列化语义清晰（构造时全部字段已知）
- 插件可扩展事件类型
- 与协同编辑的事件偏序设计天然兼容

### Negative
- 事件类构造样板较多（缓解：`MYCAD_REGISTER_EVENT` 宏 + Claude Code 模板）
- 修改事件 schema 需要走文档迁移流程（[§二 §2.9.5](../architecture/02-technical.md)）
- "更新某个字段"必须发新事件而非修改旧事件（这正是设计意图）

### Neutral
- 命名规范需要团队/AI 协作工具一致执行
- Doxygen 注释中需要说明"何时被发出"+"哪些读模型订阅"（详见 [§五 §5.2.2](../architecture/05-code-skeletons.md)）
- 事件类型字符串需要保持稳定（一旦发布不能改）

## Implementation Notes

### 基类骨架

详见 [§五 §5.2.1](../architecture/05-code-skeletons.md)：

```cpp
class DomainEvent {
public:
    const EventId        id;
    const AggregateId    aggregateId;
    const Version        version;
    const OperationId    operationId;
    const std::chrono::system_clock::time_point timestamp;
    const UserId         userId;
    const LogicalClock   vectorClock;

    [[nodiscard]] virtual std::string_view typeName() const noexcept = 0;
    virtual void serialize(IEventSerializer&) const = 0;
    virtual ~DomainEvent() = default;
};
```

### 命名约定

- 形式：`<Subject><Action(过去时)>`
- 示例：
  - ✅ `SketchEntityAdded`、`FeatureRebuilt`、`AssemblyMateBroken`
  - ❌ `AddSketchEntity`、`RebuildFeature`、`BreakMate`（命令式，不正确）

### 注释规范

每个事件类必须包含 Doxygen 注释：

```cpp
/// 草图约束被添加事件。由 Sketch::addConstraint 命令处理产生。
///
/// 何时被发出：
///   - 用户在草图工具栏点击"添加重合约束"并选择两个点
///   - 导入文件时草图约束被解析
///
/// 哪些读模型订阅：
///   - ECS Read Model 的 ConstraintComponent 创建
///   - ConstraintPropagationSystem 标记草图为 dirty
class SketchConstraintAdded final : public DomainEvent { /* ... */ };
```

### CI 检查

```yaml
- name: Verify event class names use past tense
  run: |
    # 简单 heuristic：事件类不应以动词开头（Add/Remove/Update 等）
    bad=$(grep -rE "class\s+(Add|Remove|Update|Create|Delete|Set|Modify)[A-Z]\w+\s*:.*DomainEvent" src/)
    if [ -n "$bad" ]; then
      echo "Event class names should be past tense:"
      echo "$bad"
      exit 1
    fi
```

## References

- [§二 §2.2 DDD 限界上下文](../architecture/02-technical.md)
- [§二 §2.3 事件溯源](../architecture/02-technical.md)
- [§五 §5.2 DomainEvent 骨架](../architecture/05-code-skeletons.md)
- 事件溯源经典著作：Greg Young, "Event Sourcing" 系列博客
- 相关 ADR: [ADR-0002](./ADR-0002-domain-zero-deps.md)（Domain 零依赖巩固事件不可变性）
