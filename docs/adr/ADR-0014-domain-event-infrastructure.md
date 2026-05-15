# ADR-0014: Domain Event Infrastructure and Port Interface Conventions

- **Status**: Accepted
- **Date**: 2026-05-14
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 项目维护者 (accepted)

## Context（背景）

Sprint 0.2 在实现 `DomainEvent` 基类、`EventTypeRegistry`、以及四个 port 接口（`IGeometryPort` / `IEventStore` / `IConstraintSolver` / `IEntityRegistry`）时，遇到了一批**重复出现**的设计决策，每次都要从头讨论：

- `DomainEvent` 是否允许拷贝？
- 时间戳用 `std::chrono::time_point` 还是 `uint64_t`？
- `EventTypeRegistry` 是纯单例还是可本地构造？
- 宏 `MYCAD_DOMAIN_EVENT` 是否应该内含 `public:`？
- Port 接口的 copy/move 语义是什么？
- 端口方法是否 noexcept？
- 错误如何传达（异常 vs 返回值）？

和 ADR-0013 类似：不在 Sprint 0.2 开始前钉死，每个接口都会重复这些讨论。

## Decision（决策）

### D1：DomainEvent — 不可拷贝 / 不可移动，值传参

```cpp
class DomainEvent {
public:
    DomainEvent(const DomainEvent&)            = delete;
    DomainEvent& operator=(const DomainEvent&) = delete;
    DomainEvent(DomainEvent&&)                 = delete;
    DomainEvent& operator=(DomainEvent&&)      = delete;
    ...
};
```

**理由**：事件是溯源系统的永久记录，语义上"不可复制"。禁止移动防止意外转移所有权；事件始终通过 `std::unique_ptr<DomainEvent>` 持有和传递。

### D2：时间戳用 `uint64_t` Unix ms，而非 `std::chrono`

```cpp
explicit DomainEvent(..., std::uint64_t occurredAtMs) noexcept;
```

**理由**：`<chrono>` 在 domain 层引入平台相关的 clock 类型；`uint64_t` 足够表达 UTC ms 精度，序列化/反序列化更简单，且与 ADR-0002 "domain 零外部依赖"一致。测试时调用者传入固定值，天然可复现。

### D3：EventTypeRegistry — 单例 + 可本地构造

```cpp
// 进程级单例
EventTypeRegistry& EventTypeRegistry::instance() noexcept;

// 也允许本地构造（供测试隔离）
EventTypeRegistry localReg;
```

**理由**：生产代码通过 `instance()` 全局注册；测试需要隔离的注册表以避免测试污染单例。将构造函数设为 `public` 既满足测试，也不破坏单例语义（调用者有意识地选择 local vs singleton）。

### D4：`MYCAD_DOMAIN_EVENT` 宏内含 `public:`

```cpp
#define MYCAD_DOMAIN_EVENT(type_string)                            \
public:                                                            \
    [[nodiscard]] std::string typeName() const noexcept override { \
        return type_string;                                        \
    }
```

**理由**：MSVC 下 `class` 默认访问是 `private`；若宏置于 `public:` 之前，`typeName()` 变成私有导致编译错误。宏内含 `public:` 让放置位置无关，消除调用者的心智负担。

### D5：Port 接口 — 删除拷贝，允许多态持有

```cpp
class IGeometryPort {
public:
    virtual ~IGeometryPort() = default;
    IGeometryPort(const IGeometryPort&)            = default;  // ← 允许（无状态接口）
    IGeometryPort& operator=(const IGeometryPort&) = default;
    ...
protected:
    IGeometryPort() = default;
};

class IEventStore {
public:
    IEventStore(const IEventStore&)            = delete;  // ← 禁止（代表外部资源）
    IEventStore& operator=(const IEventStore&) = delete;
    ...
};
```

**规则**：
- **无状态几何端口**（`IGeometryPort` / `IConstraintSolver`）：允许 default copy（通常无状态，函数集合）
- **有状态资源端口**（`IEventStore` / `IEntityRegistry`）：删除 copy/move（代表数据库连接 / ECS registry 等不可复制资源）

### D6：Port 方法的 noexcept 和错误传达规约

| 场景 | 处理方式 |
|---|---|
| 几何"无解"（平行线不相交） | `std::optional<T>` 返回值 |
| 乐观并发冲突（版本不匹配） | 抛 `ConcurrencyError : std::runtime_error` |
| 所有纯计算型方法 | `noexcept` |
| 资源操作（可能 OOM） | 不标 noexcept，`@throws std::bad_alloc` 文档化 |

**理由**：`std::optional` 是"无解"的自然表达，不是错误；`ConcurrencyError` 是可恢复的业务错误，适合异常。domain 不引入 `std::expected` / `tl::expected`（ADR-0002 零外部依赖；`std::expected` 是 C++23，MSVC v18 支持但不在最低约束集内）。

### D7：`IConstraintSolver` 词汇类型放在同一头文件

`SolverVariable` / `SolverConstraint` / `SolveResult` / `ConstraintKind` 全部放在 `IConstraintSolver.hpp`，不拆散到单独头文件。

**理由**：这些类型只在约束求解上下文有意义，拆散会增加 include 复杂度且无重用价值。Sprint 1.A 完整约束代数落地时，若词汇类型膨胀，届时再拆。

## Consequences（后果）

### Positive

- ✅ DomainEvent 子类实现者只需一行宏 + 构造函数，无样板代码
- ✅ 测试可以用本地 EventTypeRegistry 隔离，不需要 setUp/tearDown 清洗单例
- ✅ Port 接口规约统一：几何用 optional，资源用异常，纯计算用 noexcept
- ✅ `uint64_t` 时间戳让单元测试时间确定性可复现

### Negative

- ⚠️ **DomainEvent 不可移动**让某些 std::vector 用法需要 unique_ptr 包装 —— 这是预期的，事件天生通过 unique_ptr 传递
- ⚠️ **`uint64_t` 丢失 timezone 语义** —— 缓解：在 infrastructure 边界统一用 UTC；domain 内不做时区计算
- ⚠️ **MYCAD_DOMAIN_EVENT 宏含 `public:` 会改变后续成员访问级别** —— 缓解：按惯例在类开头使用宏后紧跟 `public:`（幂等无害）

## References

- [ADR-0002](ADR-0002-domain-zero-deps.md) — domain 零外部依赖（排除 std::chrono 作为参数类型）
- [ADR-0003](ADR-0003-events-immutable.md) — 事件不可变（D1 的上游根据）
- [ADR-0013](ADR-0013-value-object-design-philosophy.md) — 值对象哲学（D6 noexcept 规约延伸）
- 实现样板: `src/domain/shared/include/mycad/domain/DomainEvent.hpp`, `IEventStore.hpp`
