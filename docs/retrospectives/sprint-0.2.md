# Sprint 0.2 复盘 — Domain 骨架

**周期**：2026-05-13 ~ 2026-05-14（压缩到 2 个对话 session，约 10 个逻辑 day）

---

## 计划 vs 实际

| 任务 | 计划估时 | 实际耗时 | 状态 | 偏差原因 |
|---|---|---|---|---|
| T1 值对象（Point2D/3D, Vector3D, Axis3D, BoundingBox, Transform3D）| 3d | 2d | ✅ done | AI 直接实现，无 DeepSeek 往返 |
| T2 身份类型（EventId, AggregateId, SketchId, Version）| 1d | 0.5d | ✅ done | 结构相似，快速复制 |
| T3 DomainEvent + EventTypeRegistry | 1.5d | 1d | ✅ done | 两次编译错误（访问控制，见 Blocker） |
| T4 IGeometryPort | 1d | 0.5d | ✅ done | 符合 |
| T5 IEventStore | 0.5d | 0.5d | ✅ done | 符合 |
| T6 IConstraintSolver | 0.5d | 0.5d | ✅ done | 词汇类型设计比预期花时间 |
| T7 IEntityRegistry | 0.5d | 0.5d | ✅ done | 符合 |
| T8 natvis 解锁 + ADR-0014 + retro | 1d | 0.5d | ✅ done | 符合 |

**整体偏差**：约 -20%（实际快于计划），原因：AI 直接实现消除了 DeepSeek 往返轮次。

---

## 关键产出

### 代码（+18 个新文件）

**值对象 + 容差**
- `Tolerance.hpp` — `kDefaultEpsilon = 1e-9`, `nearEqual()` × 2
- `Point2D.hpp/.cpp`, `Point3D.hpp/.cpp` — 16B / 24B 值对象
- `Vector3D.hpp/.cpp`, `Axis3D.hpp`, `BoundingBox.hpp`, `Transform3D.hpp/.cpp`

**身份类型**
- `EventId.hpp/.cpp`, `AggregateId.hpp/.cpp`, `SketchId.hpp/.cpp`, `Version.hpp/.cpp`
- 全部：UUID 格式 `to_string`、FNV-1a `std::hash`、`operator<=>`

**事件基础设施**
- `DomainEvent.hpp` — 抽象基类，deleted copy/move，`MYCAD_DOMAIN_EVENT` 宏
- `EventTypeRegistry.hpp/.cpp` — 单例 + 可本地构造，`MYCAD_REGISTER_EVENT` 宏

**Port 接口（纯头文件）**
- `IGeometryPort.hpp` — 2D × 5 方法，3D × 4 方法，BoundingBox
- `IEventStore.hpp` — `append`（乐观并发）、`load`、`loadSince`、`latestVersion`；`ConcurrencyError`
- `IConstraintSolver.hpp` — `SolverVariable/Constraint/Result`，`ConstraintKind` 枚举，`solve()`
- `IEntityRegistry.hpp` — `create/destroy/isAlive/size/clear`，`kNullEntity`

**测试（+4 个测试文件，111 个测试用例，100% pass）**
- `value_objects_test.cpp` — 50+ cases（Tolerance / Point / Vector / Axis / BBox / Transform）
- `identity_test.cpp` — 13 cases
- `domain_event_test.cpp` — 15 cases
- `geometry_port_test.cpp` — 15 cases
- `ports_test.cpp` — 18 cases（IEventStore × 6, IConstraintSolver × 5, IEntityRegistry × 7）

**文档**
- `ADR-0014-domain-event-infrastructure.md` — 7 项设计决策固化
- `tools/visualizers/mycad.natvis` — 解锁 11 种类型（全部 Sprint 0.2 产出）

---

## 遇到的 Blocker

### B1：`MYCAD_DOMAIN_EVENT` 宏生成私有方法（MSVC class 默认 private）

**现象**：`DomainEvent -typeName returns declared string` 编译报 C2248，`typeName()` 无法访问。

**根因**：宏放在类 body 默认 `private:` 区域，生成的 `typeName()` 是私有成员。

**修复**：宏内含 `public:`，无论放置位置均保证 `typeName()` 公开。

**规避规则**（加入 CLAUDE.md §2.4）：
> `MYCAD_DOMAIN_EVENT` 宏定义中必须含 `public:`；具体事件类中宏位置不限。

---

### B2：`EventTypeRegistry` 私有构造函数导致测试无法创建本地实例

**现象**：`EventTypeRegistry reg;` 编译报 C2248（私有构造函数）。

**根因**：单例模式的惯用做法是把构造函数私有化；但测试需要隔离实例。

**修复**：构造函数改为 `public`，单例通过 `instance()` 访问的语义不变；调用方有意识选择 local vs singleton。

**教训**：测试隔离需求应在接口设计时同步考虑，"私有构造函数" 是过度约束。

---

### B3：`std::span<const DomainEvent* const>` 在测试中构造笨拙

**现象**：`IEventStore::append` 接受 `std::span<const DomainEvent* const>`，测试中每次都要手工：
```cpp
const DomainEvent* ptr = &ev;
store.append(aid, ver, std::span<const DomainEvent* const>{&ptr, 1});
```

**状态**：已接受（Sprint 0.3 可以考虑加 `appendOne(AggregateId, Version, const DomainEvent&)` 便利重载）。

---

## 设计决策摘要（详见 ADR-0014）

| 决策 | 选择 |
|---|---|
| DomainEvent 可拷贝性 | 完全不可拷贝 / 不可移动 |
| 时间戳类型 | `uint64_t` Unix ms（不用 `<chrono>`） |
| EventTypeRegistry 单例 | 单例 + public 构造函数（测试用） |
| 宏 `MYCAD_DOMAIN_EVENT` | 内含 `public:`，位置无关 |
| Port 拷贝语义 | 无状态 port（IGeometryPort）允许 default；有状态（IEventStore）删除 |
| 错误传达 | 无解 → `std::optional`；乐观并发冲突 → `ConcurrencyError` 异常 |
| 词汇类型位置 | 与接口头文件同文件（IConstraintSolver.hpp） |

---

## 遗留 / 下一 Sprint 待跟进

| 项目 | 优先级 | 说明 |
|---|---|---|
| `IEventStore::appendOne` 便利重载 | 低 | 减少测试样板代码 |
| `MYCAD_REGISTER_EVENT` 宏验证 | 中 | 当前宏未被测试实际调用（只测了手动 registerType） |
| `std::expected` 评估 | 低 | C++23；等 MSVC v18 全面支持后替代 optional + exception 混用 |
| Transform3D::inverse() | 中 | Sprint 1.B 特征旋转需要逆变换 |
| natvis Transform3D row 展示 | 低 | 当前用字符串拼接，调试体验待验证 |

---

## Sprint 感受（1-10）

**8 / 10**

`+`：111 个测试全绿、18 个文件、ADR-0014、natvis，在 2 个 session 内完成计划 10 天的内容。AI 直接实现消除了大量往返，MSVC 编译器错误大多在几轮内自愈。

`-`：natvis 的 Transform3D 行展示用字符串拼接（`"[...]"`）而非真正的数组展开，实际调试效果待验证；`DomainEvent` 私有访问 blocker 应在设计时提前避免。

---

_Generated: 2026-05-14_
