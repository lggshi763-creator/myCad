# ADR-0013: 值对象设计哲学

- **Status**: Accepted
- **Date**: 2026-05-06
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 项目维护者 (accepted as-is, 9/9 决策)

## Context（背景）

Sprint 0.2 第一项任务"实现核心值对象（`Point2D` / `Point3D` / `Vector3D` / `Transform3D` / `Axis3D` / `BoundingBox`）"涉及一系列**横切**的设计决策，每个值对象都会撞到同一组问题：

- 传值还是传引用？
- 不变性策略？
- 怎么比较（特别是浮点）？
- 容差怎么处理？
- 数值类型固定 `double` 还是模板化？
- 单位是隐式 mm 还是强类型 `Length<mm>`？
- 是否提供 `std::hash` 特化？
- `noexcept` 怎么标？
- 怎么调试输出（不能用 `<format>` / `fmt`）？

**不在 Sprint 0.2 启动前钉死，每个值对象都会反复重复这些讨论**，到装配体（Phase 2）就第三轮了。

不决策的代价：值对象 5+ 个 + 后续 ID 类型 4+ 个 + 装配相关值对象，至少 15+ 处会撞同样的问题，每次 30 分钟讨论 = ~7 小时浪费。

## Decision（决策）

我们决定 **9 项设计哲学**如下表（详见各小节理由）。**所有 domain 层值对象一律遵守**。

| # | 决策项 | 选择 |
|---|---|---|
| 1 | 传值 vs 引用 | **< 32 字节传值，≥ 32 字节传 const ref** |
| 2 | 不变性 | **构造后只读**，无 setter；变换返回新实例 |
| 3 | 比较语义 | **C++20 `operator<=>` 默认**；浮点类**显式 `operator==` 走 epsilon 容差** |
| 4 | 浮点容差 | **全局默认 `kDefaultEpsilon = 1e-9`** + 单参 `nearEqual(a, b)` + 三参 `nearEqual(a, b, tol)` 覆盖 |
| 5 | 数值类型 | **`double` 固定**，不模板化、不用 `float` |
| 6 | 单位 | **隐式 mm 全域**，I/O 边界做转换；强类型 `Length<U>` 推迟到 Phase 2 评估 |
| 7 | `std::hash` 特化 | **仅 ID 类型提供**（EventId / AggregateId 等）；几何值对象**不提供** |
| 8 | `noexcept` 边界 | **全 noexcept**（domain 值对象本就 trivial、零分配） |
| 9 | 调试输出 | **自由函数 `to_string(const Point2D&)`**（同命名空间）；不用 `operator<<`、不用成员方法、不用 `<format>` |

## Considered Alternatives（候选方案与理由）

### 决策 1：传值 vs 引用

**选 < 32 字节传值，≥ 32 字节传 const ref。**

| 类型 | 大小 | 传递方式 |
|---|---|---|
| `Point2D` (2× double) | 16 B | 传值 |
| `Point3D` (3× double) | 24 B | 传值 |
| `Vector3D` (3× double) | 24 B | 传值 |
| `Axis3D` (Point3D + Vector3D) | 48 B | const ref |
| `BoundingBox` (2× Point3D) | 48 B | const ref |
| `Transform3D` (4×4 matrix = 16× double) | 128 B | const ref |
| `EventId` / 各种 ID (16 bytes UUID) | 16 B | 传值 |

**理由**：
- < 32 B 传值在所有现代 ABI 下命中寄存器（x64 SysV / Windows x64 都允许小 struct 在 寄存器返回）
- ≥ 32 B 传值会落到 stack，传引用更清晰
- NRVO / RVO 让"返回值"路径无 copy 开销，统一传值在出参侧无损失

**拒绝**：
- "全部传 const ref"：小对象损失寄存器优化、丢失 RVO；
- "全部传值"：Transform3D 在递归调用栈深时炸 stack。

### 决策 2：不变性

**值对象**构造后**只读**，无 setter。需要"修改"的语义改为**返回新实例**：

```cpp
Point2D p{1.0, 2.0};
Point2D q = p.translated({3.0, 4.0});  // ✅ p 不变，q 新建
p.translate({3.0, 4.0});                // ❌ 没有这种 API
```

**理由**：
- 不变性 + 事件溯源天然契合（事件记录"状态转变"，值本身不变）
- 编译期就能保证线程安全（const 不可变即可在线程间共享）
- 与 ADR-0003 "事件不可变" 一致语义

**拒绝**：
- "提供 setter"：增加状态共享和别名问题，破坏 thread-safe-by-default

### 决策 3：比较语义

**纯整型 / ID 类型**：C++20 `operator<=>` 默认（`= default`）。
**浮点类（Point / Vector / Transform 等）**：**禁用** `<=>` 默认（bitwise 比较有 NaN / ±0 / 误差陷阱），**显式定义** `operator==` 走 epsilon 容差。

```cpp
// ID 类型
struct EventId {
    std::array<std::uint8_t, 16> bytes;
    auto operator<=>(const EventId&) const = default;  // ✅
};

// 浮点几何
struct Point2D {
    double x;
    double y;
    // 不要 = default 的 <=>；显式定义 ==
    [[nodiscard]] friend bool operator==(Point2D a, Point2D b) noexcept {
        return nearEqual(a.x, b.x) && nearEqual(a.y, b.y);
    }
    // 不提供 <=> —— 几何点没有自然全序
};

// 如果罕见地需要"逐位精确比较"（如序列化往返测试），用具名函数：
[[nodiscard]] bool bitwiseEqual(Point2D a, Point2D b) noexcept;
```

**理由**：
- 浮点 `<=>` 默认行为反直觉（`-0.0 != 0.0` / `NaN != NaN`），调试地狱
- 几何点天生**无全序**（"哪个点"小"？"），强行定义 `<` 是滑坡
- ID 是字节串，bitwise `<=>` 是正确的

**拒绝**：
- "全部 = default `<=>`"：浮点失败模式见上
- "全部显式 =="：ID 类型反而需要排序（map key），写多余

### 决策 4：浮点容差

**全局默认 + 可覆盖**。

```cpp
// 在 mycad/domain/Tolerance.hpp 中
namespace mycad::domain {

/// CAD 工程级默认容差。mm-scale 几何下 1e-9 mm = 1 fm，远低于实际制造公差。
inline constexpr double kDefaultEpsilon = 1e-9;

/// @brief Returns true if |a - b| <= kDefaultEpsilon.
[[nodiscard]] constexpr bool nearEqual(double a, double b) noexcept;

/// @brief Returns true if |a - b| <= tolerance.
[[nodiscard]] constexpr bool nearEqual(double a, double b, double tolerance) noexcept;

}  // namespace mycad::domain
```

**理由**：
- 强类型 `Tolerance` value object 在 MVP 阶段是 over-engineering（每次写 `Tolerance{1e-9}` 而非 `1e-9` —— API 噪声大于价值）
- SolidWorks / Creo 内部都用 hardcoded epsilon + 几个用户可配的级别（"严格"/"标准"/"宽松"），MVP 不必走那么远
- 1e-9 在 mm-scale 适用，到 Phase 2 装配跨尺度（米级到微米级）才需要重新评估

**拒绝**：
- "强类型 `Tolerance`"：复杂度成本超过 MVP 价值，留待装配体阶段
- "每次显式传"：调用点噪声大，遗漏率高
- "纯全局，无覆盖"：拒绝灵活性，遇到刻意要严的检查（如 fuzz 测试）会卡

### 决策 5：数值类型固定 `double`

**全部几何用 `double`，不模板化、不用 `float`。**

```cpp
struct Point3D {
    double x, y, z;  // ✅ 固定 double
};

// ❌ 不要 template<typename T> struct Point3D
// ❌ 不要 float
```

**理由**：
- CAD 工程几何对精度敏感 —— `float` 在大尺寸（米级建模）+ 小特征（孔位 0.01mm）下精度不够，已是工业共识
- 模板化几何类（pmp-library / CGAL 风格）在通用库有价值；myCad 是**领域**而非**通用库**，不需要
- 模板化让所有 .hpp 必须 inline 或 explicit-instantiate，编译时间膨胀（OCCT 那种 30s 头文件展开就是教训）

**拒绝**：
- `float`：精度不够
- `template<typename T>`：复杂度无收益

### 决策 6：单位 — 隐式 mm 全域

**所有 domain 层数值假定为 mm**。I/O 边界（CAD 文件解析、UI 输入）负责单位转换。

```cpp
// ✅ Domain 内部 — 数字就是 mm
Point3D p{10.5, 20.3, 5.1};            // 10.5mm, 20.3mm, 5.1mm
double area = computeFaceArea(face);    // 单位 mm²

// ✅ I/O 边界做转换
namespace mycad::infrastructure {
constexpr double kInchToMm = 25.4;
double mmFromInch(double inches) noexcept { return inches * kInchToMm; }
}

// ❌ 不要 — 强类型 Length 推迟评估
Length<Millimeter> p{10.5};              // 等 Phase 2 再考虑
```

**理由**：
- 99% 的工程 CAD 默认 mm（中国 GB / 欧洲 ISO 都是）；强类型对主流用例只是噪声
- 强类型 `Length<U>` 加每个参数 `Length{42.0}` 字符 → 整个 API 视觉噪声 +30%
- 明确写在 ADR 让所有人知道这条隐式约定，比强类型更轻
- Phase 2 装配体如果需要跨单位（imperial / metric mix），届时**单点引入** `Length<U>`，所有现有代码不用动 —— 风险可控

**拒绝**：
- 强类型单位 `Length<mm>`：MVP 阶段成本 > 收益
- 全 SI 单位（米）：与 CAD 行业惯例（mm）相悖

**红线**：domain 内任何函数参数 / 返回值 / 数据成员是数值时，注释或文档**必须明示是 mm**（默认就是 mm；非 mm 必须特殊标注）。

### 决策 7：`std::hash` 特化

**仅 ID 类型提供**：`EventId` / `AggregateId` / `SketchId` / `FeatureId` / `Version`。
**几何值对象不提供**：`Point2D` / `Point3D` / `Vector3D` / `Transform3D` / `Axis3D` / `BoundingBox`。

```cpp
// ✅ ID 类型 — 必有 hash（用作 map / set key）
namespace std {
template<>
struct hash<mycad::domain::EventId> {
    std::size_t operator()(const mycad::domain::EventId& id) const noexcept;
};
}

// ❌ 几何值对象 — 不要 hash
// 几何值用 epsilon 比较，直接 hash bytes 会导致"相等但 hash 不同"，违反 hash 契约
```

**理由**：
- ID 类型必有 hash，因为 EventStore / EntityRegistry 用 hashmap 索引
- 几何值的 epsilon 比较与 hash 强契约不兼容（`a == b` 必须 `hash(a) == hash(b)`，但 epsilon 无法生成稳定 hash）
- "用几何值做 map key"是 anti-pattern —— 应该用 ID 或离散化（grid bucket）替代

**拒绝**：
- 全部提供：见上，几何值 hash 不安全
- 全部不提供：ID 类型必须支持 unordered map

### 决策 8：`noexcept` 边界

**所有值对象方法 `noexcept`**（构造、比较、变换、debug 输出全部）。

```cpp
struct Point3D {
    double x, y, z;

    // ✅ trivial constructor — 自然 noexcept
    Point3D(double x, double y, double z) noexcept;

    // ✅ 算术变换 — 纯数学，无分配
    [[nodiscard]] Point3D translated(Vector3D v) const noexcept;

    // ✅ 比较 — 纯比较
    [[nodiscard]] friend bool operator==(Point3D a, Point3D b) noexcept;
};
```

**理由**：
- Domain 零依赖（ADR-0002）+ 值对象通常不分配堆内存 → 自然 noexcept
- `noexcept` 启用更激进的编译器优化（move > copy 选择路径）
- 调用者 / 测试更清晰（不需要 `try/catch` 包裹纯算术）

**例外**：
- 仅当一个方法**真的可能**抛（如内部用 `std::vector` 暂存中间结果且分配可能 OOM），明确**不**标 `noexcept`，并用 `@throws std::bad_alloc` 文档化
- `to_string` 返回 `std::string`，理论上分配可能 OOM —— **不**标 noexcept，按 §9 处理

**拒绝**：
- 全部不标 noexcept：放弃优化和清晰性
- "看心情标"：缺一致性，团队约定不清

### 决策 9：调试输出

**自由函数 `to_string(const T&)`**（**同命名空间**），返回 `std::string`，**纯字符串拼接实现**（不用 `<format>` / `fmt`）。

```cpp
namespace mycad::domain {

/// @brief Human-readable representation for logs and debuggers.
///
/// 形如 "Point3D{x=1.5, y=2.5, z=3.5}"。仅用于 debug / log，不是序列化格式。
/// 序列化走 events/ 下的 FlatBuffers schema。
[[nodiscard]] std::string to_string(Point3D p);

}  // namespace mycad::domain
```

**实现样板**（参考 `Hello.cpp`，纯 `std::string` 拼接）：

```cpp
std::string to_string(Point3D p) {
    std::string out;
    out.reserve(48);
    out.append("Point3D{x=");
    out.append(std::to_string(p.x));   // 标准库 to_string，不需 fmt
    out.append(", y=");
    out.append(std::to_string(p.y));
    out.append(", z=");
    out.append(std::to_string(p.z));
    out.append("}");
    return out;
}
```

**理由**：
- 自由函数（同命名空间，ADL 可用）— 不耦合到 `<iostream>`（`operator<<` 强行依赖）
- 不用 `<format>` / `fmt::format` — 严格 ADR-0002（domain 零外部依赖）
- 不用成员 `toString()` — 强制接口 vs 可选自由函数；后者更 C++ 风格
- `std::to_string(double)` 输出格式不完美但够 debug；**不在 to_string 里追求漂亮**，UI 渲染是 ui/ 层的职责

**拒绝**：
- `operator<<(std::ostream&)`：拉入 `<iostream>` / `<ostream>`，违反 ADR-0002
- 成员 `toString()`：所有值对象强行带这个方法，接口刚性
- `<format>` / `fmt::format`：ADR-0002 红线
- "不提供 debug 输出"：调试时 natvis 不一定全覆盖，自由函数总能 fallback

## Rationale（为何这套搭配）

- **保守优先**：MVP 阶段拒绝过度抽象（拒绝模板化数值、拒绝强类型单位、拒绝强类型 Tolerance）—— 这些都是"以后可能需要"，但**现在必然**增加 API 噪声
- **一致性优先**：9 条规则覆盖**所有**值对象，无例外可寻；新值对象作者**不需要决策**，照搬即可
- **与现有 ADR 全兼容**：
  - ADR-0002（domain 零依赖）→ 决策 9 用纯 std::string；决策 6 不引入单位库
  - ADR-0003（事件不可变）→ 决策 2 不变性同向
  - ADR-0010（drawing 是独立聚合）→ 决策 1-9 同样适用 drawing 内的值对象（Sheet / View / Dimension）

## Consequences（后果）

### Positive

- ✅ Sprint 0.2 第一周可以"无脑实现"6 个值对象，无设计争论
- ✅ 任何新增值对象（装配体 / 工程图标注等）都套同一模板，团队心智模型统一
- ✅ DeepSeek 实现新值对象时 prompt 可以引用"按 ADR-0013 风格" 一句话搞定，不需逐项澄清
- ✅ 浮点比较的"epsilon 陷阱"在 ADR 层提前避开，Sprint 0.2 不会撞这个

### Negative

- ⚠️ **隐式 mm 是潜在地雷**：将来如果要支持 imperial 单位作为一等公民，要么彻底重构（painful），要么继续 I/O 边界转换（功能 OK 但体验割裂）—— 缓解：在 Phase 2 进入装配体 / 互操作时，专门写一个 ADR 评估"是否要 `Length<U>`"
- ⚠️ **没有强类型 Tolerance 让"严格 / 宽松"配置只能用魔法数字** —— 缓解：Phase 1 末（草图约束求解器接入时）评估是否升级
- ⚠️ **几何值不提供 hash 限制了某些数据结构选择** —— 缓解：本来就是 anti-pattern，不损失实际用例

### Neutral

- 🔧 需要先实现 `mycad/domain/Tolerance.hpp`（决策 4）作为其他值对象的依赖
- 🔧 [docs/architecture/05-code-skeletons.md](../architecture/05-code-skeletons.md) 需要更新值对象的代码骨架示例，引用本 ADR
- 🔧 后续 ADR-0014+ 如果需要反转某条决策，必须 supersede 本 ADR 的对应小节

## Implementation Notes（实施注记）

### 第一波落地顺序（Sprint 0.2 Day 1-2）

1. `mycad/domain/Tolerance.hpp` —— `kDefaultEpsilon` + `nearEqual` × 2 重载
2. `mycad/domain/Point2D.hpp` + `.cpp` —— 全套：构造、`operator==`、`translated`、`to_string`
3. `mycad/domain/Point3D.hpp` + `.cpp` —— 同上
4. `mycad/domain/Vector3D.hpp` + `.cpp` —— 加 `dot` / `cross` / `length` / `normalized`
5. `mycad/domain/Axis3D.hpp` —— 由 Point3D + Vector3D 组合
6. `mycad/domain/BoundingBox.hpp` —— 由两个 Point3D 组合
7. `mycad/domain/Transform3D.hpp` + `.cpp` —— 4×4 矩阵；`apply(point)` / `apply(vector)` / `composed`

每个都附 Catch2 单测（覆盖率 ≥ 90%，包括 epsilon 边界 / 极端值 / NaN 行为）。

### Code Review Checklist（Sprint 0.2 PR 验收用）

- [ ] 头文件路径：`src/domain/shared/include/mycad/domain/...`
- [ ] include：`<mycad/domain/X.hpp>`
- [ ] 结构体大小 ≥ 32 B 时所有 API 改成 const ref
- [ ] 所有方法 `noexcept`（除 to_string）
- [ ] 浮点比较走 `nearEqual`，不要直接 `==`
- [ ] 提供 `to_string`（自由函数，**纯 std::string 拼接**）
- [ ] 不提供 `std::hash` 特化（除非是 ID 类型）
- [ ] `[[nodiscard]]` 标在所有返回值有意义的方法
- [ ] Doxygen 注释：英文 brief + 中文详情 + `@throws`（如果有）+ `@thread-safe`（值对象天然是）
- [ ] Catch2 单测含 epsilon 边界 + 默认构造 + 极端值

### 迁移路径（如果未来反转某条决策）

| 决策 | 反转代价 |
|---|---|
| 1 传值/引用 | 低 — 全部改 API 签名，编译器辅助 |
| 2 不变性 | 中 — 加 setter 后调用方可能依赖，回退困难 |
| 3 比较 | 低 — 局部修改 |
| 4 容差 | 中 — 引入 `Tolerance` 强类型涉及多处 |
| 5 数值类型 | 高 — 模板化全部值对象是大手术 |
| 6 单位 | **极高** — 隐式 mm → 强类型是项目级重构 |
| 7 hash | 低 — 加特化即可 |
| 8 noexcept | 低 — 去除标注 |
| 9 to_string | 低 — 同命名空间增加方法不破坏调用方 |

**警告**：决策 5 和 6 是**最难反转**的；现在选定就接受这个未来代价。

## References（参考）

- 相关 ADR: [ADR-0002](ADR-0002-domain-zero-deps.md)（domain 零依赖）, [ADR-0003](ADR-0003-events-immutable.md)（事件不可变）, [ADR-0010](ADR-0010-drawing-domain-boundary.md)（drawing 同样适用）
- 相关文档: [docs/architecture/05-code-skeletons.md](../architecture/05-code-skeletons.md), [src/domain/shared/include/mycad/domain/Hello.hpp](../../src/domain/shared/include/mycad/domain/Hello.hpp)（实现风格样板）
- 外部参考:
  - C++ Core Guidelines [F.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f16-for-in-parameters-pass-cheaply-copied-types-by-value-and-others-by-reference-to-const)（小值传值，大值传 const ref）
  - C++ Core Guidelines [C.20-44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#cconcrete-concrete-types)（concrete types / value semantics）
  - SolidWorks / Creo 内部容差策略（行业惯例）
  - boost::units（强类型单位库案例 — 不采用但参考其 API 噪声警告）

---

## Acceptance Log（接受记录）

- **2026-05-06**：项目维护者逐条审阅，**9 项全部按起草版本接受**，无修订。Status 从 `Proposed` → `Accepted`。
- 评估时机：决策 #5（`double` 固定）和 #6（隐式 mm）是难以反转的；约定在 **Phase 2 装配体 / 互操作 sprint 启动前**重新评估一次，确认 myCad 目标用户画像（工业机械 / 军工 / 央企研发）仍以 mm-scale 为主。如需切换，再写 ADR-0014/15 supersede 对应小节。
