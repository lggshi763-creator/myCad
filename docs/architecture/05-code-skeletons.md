# §五 核心代码骨架

> 回到导航：[../../ARCHITECTURE.md](../../ARCHITECTURE.md)

> 全部代码遵循 C++20 现代惯用法：concepts、ranges、`std::expected`（C++23 polyfill：`tl::expected`）、RAII 严格、`[[nodiscard]]` 强制、PascalCase 类 / camelCase 方法 / 成员尾下划线 `_`。
>
> 所有头文件均在 `namespace mycad::<layer>::<module>` 下。本节示例为简洁起见省略部分嵌套命名空间。
>
> 这些骨架是 Phase 0 Sprint 0.2-0.4 的实际交付物。每个接口对应 [§四 4.1.3](./04-tech-decisions.md) 的一个 Tier A 抽象。

---

## 5.1 `IGeometryPort` — 几何内核端口接口

**位置**：`src/domain/shared/include/mycad/domain/IGeometryPort.hpp`

**设计意图**：六边形架构 Port 端 — 描述领域层"需要什么几何能力"，与具体实现（OCCT / 未来其他内核）解耦。所有 OCCT 类型在此层不可见。

详见 [§九 §9.2.1 OCCT → 自研路径](./09-self-host-strategy.md)。

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>      // C++23, polyfill: tl/expected.hpp
#include <span>
#include <string_view>
#include <vector>

#include "mycad/domain/values/BoundingBox.hpp"
#include "mycad/domain/values/Point3D.hpp"
#include "mycad/domain/values/Vector3D.hpp"
#include "mycad/domain/values/Axis3D.hpp"
#include "mycad/domain/values/Transform3D.hpp"
#include "mycad/domain/values/TriangleMesh.hpp"

namespace mycad::domain {

// 不透明几何句柄。Adapter 内部维护 ID → OCCT TopoDS_Shape 的映射。
// Domain 层永远不知道 OCCT 的存在。
struct BRepHandle {
    std::uint64_t id{0};
    [[nodiscard]] bool valid() const noexcept { return id != 0; }
    auto operator<=>(const BRepHandle&) const = default;
};

struct WireHandle {
    std::uint64_t id{0};
    [[nodiscard]] bool valid() const noexcept { return id != 0; }
    auto operator<=>(const WireHandle&) const = default;
};

// 拓扑引用：指向 BRep 内部某条边/面/顶点
struct EdgeRef  { BRepHandle owner; std::uint32_t index; };
struct FaceRef  { BRepHandle owner; std::uint32_t index; };
struct VertexRef{ BRepHandle owner; std::uint32_t index; };

// 业务级错误码。OCCT 异常被 Adapter 翻译到此。
enum class GeomErrorKind {
    InvalidInput,
    AlgorithmFailed,        // OCCT 算法返回失败
    DegenerateGeometry,     // 退化几何（零面积、重合点）
    BooleanFailure,         // 布尔运算失败（非流形、自相交）
    OutOfMemory,
    Unknown,
};

struct GeomError {
    GeomErrorKind kind;
    std::string  message;   // 人可读诊断
    std::string  occtTrace; // 调试用：OCCT 内部错误信息（仅 debug 构建填充）
};

struct TessellationParams {
    double linearDeflection{0.1};   // mm
    double angularDeflection{0.5};  // rad
    bool   relative{false};
};

template <typename T>
using GeomResult = std::expected<T, GeomError>;

class IGeometryPort {
public:
    virtual ~IGeometryPort() = default;

    // ---- 基础形体 ----
    [[nodiscard]] virtual GeomResult<BRepHandle>
        makeBox(double dx, double dy, double dz) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        makeCylinder(double radius, double height) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        makeSphere(double radius) = 0;

    // ---- 草图 → Wire ----
    // SketchSnapshot 是 Domain 层的轻量草图数据快照（不是聚合）
    [[nodiscard]] virtual GeomResult<WireHandle>
        makeWireFromSketch(const SketchSnapshot& sketch) = 0;

    // ---- 拉伸 / 旋转 / 扫掠 ----
    [[nodiscard]] virtual GeomResult<BRepHandle>
        prismaticExtrude(WireHandle wire, Vector3D direction, double depth) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        revolve(WireHandle wire, Axis3D axis, double angleRad) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        sweep(WireHandle profile, WireHandle path) = 0;

    // ---- 布尔 ----
    [[nodiscard]] virtual GeomResult<BRepHandle>
        booleanUnion(BRepHandle a, BRepHandle b) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        booleanCut(BRepHandle a, BRepHandle b) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        booleanIntersect(BRepHandle a, BRepHandle b) = 0;

    // ---- 倒角 / 圆角 ----
    [[nodiscard]] virtual GeomResult<BRepHandle>
        fillet(BRepHandle shape, std::span<const EdgeRef> edges, double radius) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        chamfer(BRepHandle shape, std::span<const EdgeRef> edges, double distance) = 0;

    // ---- 拓扑查询 ----
    [[nodiscard]] virtual std::vector<EdgeRef>   edges(BRepHandle)  = 0;
    [[nodiscard]] virtual std::vector<FaceRef>   faces(BRepHandle)  = 0;
    [[nodiscard]] virtual std::vector<VertexRef> vertices(BRepHandle)= 0;

    // ---- 测量 ----
    [[nodiscard]] virtual double      volume(BRepHandle) = 0;
    [[nodiscard]] virtual double      area(BRepHandle)   = 0;
    [[nodiscard]] virtual BoundingBox bbox(BRepHandle)   = 0;

    // ---- 网格化（用于渲染） ----
    [[nodiscard]] virtual GeomResult<TriangleMesh>
        tessellate(BRepHandle, TessellationParams params) = 0;

    // ---- 变换 ----
    [[nodiscard]] virtual GeomResult<BRepHandle>
        transformed(BRepHandle, const Transform3D&) = 0;

    // ---- 序列化（用于 EventStore 快照） ----
    [[nodiscard]] virtual std::vector<std::byte> serialize(BRepHandle) = 0;
    [[nodiscard]] virtual GeomResult<BRepHandle>
        deserialize(std::span<const std::byte>) = 0;

    // ---- 生命周期管理 ----
    // BRep 内部资源由 Port 管理；调用方完成使用必须 release
    virtual void release(BRepHandle) noexcept = 0;
    virtual void release(WireHandle) noexcept = 0;
};

}  // namespace mycad::domain
```

**关键设计要点**：
- 全部 `[[nodiscard]]` + `std::expected` → 编译期强制错误处理
- `BRepHandle` 是 64 位 ID 而非指针 → 支持序列化、跨进程传递（远期协同）、避免 OCCT 智能指针泄漏
- 接口故意"业务化"（`prismaticExtrude` 而非 `BRepPrimAPI_MakePrism`）→ Adapter 必须翻译，强化解耦
- `release` 显式声明 → 配合 `BRepGeometryComponent` 析构使用，零隐式生命周期

---

## 5.2 `DomainEvent` 基类 + `SketchConstraintAddedEvent` 派生

**位置**：
- `src/domain/shared/include/mycad/domain/DomainEvent.hpp`
- `src/domain/sketch/include/mycad/domain/sketch/events/SketchConstraintAdded.hpp`

**设计意图**：所有领域事件的根基。事件不可变、可序列化、自描述类型名（用于 EventStore 反序列化分发）。

### 5.2.1 基类

```cpp
#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "mycad/domain/identity/AggregateId.hpp"
#include "mycad/domain/identity/EventId.hpp"
#include "mycad/domain/identity/OperationId.hpp"
#include "mycad/domain/identity/UserId.hpp"
#include "mycad/domain/identity/AggregateType.hpp"
#include "mycad/domain/clock/LogicalClock.hpp"

namespace mycad::domain {

// 事件序列化接口。具体格式在 Infrastructure 层（FlatBuffers Adapter）实现。
class IEventSerializer {
public:
    virtual ~IEventSerializer() = default;
    virtual void writeBytes(std::span<const std::byte>) = 0;
    virtual void writeString(std::string_view) = 0;
    virtual void writeInt64(std::int64_t) = 0;
    virtual void writeDouble(double) = 0;
    // ...
};

class IEventDeserializer { /* 对偶接口 */ };

// 事件版本号。聚合内单调递增，乐观并发控制的核心。
struct Version {
    std::uint64_t value{0};
    auto operator<=>(const Version&) const = default;
    [[nodiscard]] Version next() const noexcept { return {value + 1}; }
};

// 所有 DomainEvent 的不可变基类。
//
// 为什么用基类（OO）而非 std::variant：
//   - 事件类型在编译期"开放"（插件可定义自己的事件类型）
//   - variant 在编译期固定一组类型，无法被插件扩展
//   - 性能差异微乎其微（每事件 ~100ns 多态调用）
class DomainEvent {
public:
    // ---- 不可变元数据（构造时填充，永不修改） ----
    const EventId        id;
    const AggregateId    aggregateId;
    const AggregateType  aggregateType;
    const Version        version;
    const OperationId    operationId;     // 同一用户操作的事件共享此 ID
    const std::chrono::system_clock::time_point timestamp;
    const UserId         userId;
    const LogicalClock   vectorClock;     // 协同准备：CRDT-friendly

    // ---- RTTI 替代：每个具体事件类暴露唯一类型名 ----
    // 用于 EventStore 反序列化时分发到正确的工厂函数
    [[nodiscard]] virtual std::string_view typeName() const noexcept = 0;

    // ---- 序列化 ----
    virtual void serialize(IEventSerializer&) const = 0;

    virtual ~DomainEvent() = default;

protected:
    DomainEvent(EventId eid, AggregateId aid, AggregateType atype,
                Version v, OperationId opid, UserId uid,
                LogicalClock clock,
                std::chrono::system_clock::time_point ts =
                    std::chrono::system_clock::now())
        : id(eid), aggregateId(aid), aggregateType(atype),
          version(v), operationId(opid), timestamp(ts),
          userId(uid), vectorClock(std::move(clock)) {}
};

// 工厂注册：每个事件类型注册自己的反序列化构造函数
class EventTypeRegistry {
public:
    using Factory = std::function<std::unique_ptr<DomainEvent>(IEventDeserializer&)>;

    static EventTypeRegistry& instance();

    void registerType(std::string_view typeName, Factory factory);

    [[nodiscard]] std::unique_ptr<DomainEvent>
        deserialize(std::string_view typeName, IEventDeserializer& des) const;

private:
    std::unordered_map<std::string, Factory> factories_;
};

// 注册宏：让派生类一行代码完成注册
#define MYCAD_REGISTER_EVENT(EventClass)                                   \
    namespace { struct EventClass##_Registrar {                            \
        EventClass##_Registrar() {                                         \
            ::mycad::domain::EventTypeRegistry::instance().registerType(   \
                EventClass::kTypeName,                                     \
                [](::mycad::domain::IEventDeserializer& d) {               \
                    return std::unique_ptr<::mycad::domain::DomainEvent>(  \
                        EventClass::deserializeFrom(d));                   \
                });                                                        \
        }                                                                  \
    } g_##EventClass##_registrar; }

}  // namespace mycad::domain
```

### 5.2.2 派生事件示例

```cpp
#pragma once

#include "mycad/domain/DomainEvent.hpp"
#include "mycad/domain/sketch/SketchId.hpp"
#include "mycad/domain/sketch/SketchConstraintSpec.hpp"

namespace mycad::domain::sketch {

// 草图约束被添加事件。由 Sketch::addConstraint 命令处理产生。
//
// 何时被发出：
//   - 用户在草图工具栏点击"添加重合约束"并选择两个点
//   - 导入文件时草图约束被解析
//
// 哪些读模型订阅：
//   - ECS Read Model 的 ConstraintComponent 创建
//   - 约束求解器触发：ConstraintPropagationSystem 标记草图为 dirty
class SketchConstraintAdded final : public DomainEvent {
public:
    static constexpr std::string_view kTypeName = "sketch.ConstraintAdded";

    // ---- 业务字段（不可变） ----
    const SketchId             sketchId;
    const SketchConstraintId   constraintId;
    const SketchConstraintSpec spec;

    SketchConstraintAdded(EventId eid, AggregateId aid, Version v,
                          OperationId opid, UserId uid, LogicalClock clock,
                          SketchId sid, SketchConstraintId cid,
                          SketchConstraintSpec sp)
        : DomainEvent(eid, aid, AggregateType::Sketch, v, opid, uid,
                      std::move(clock)),
          sketchId(sid), constraintId(cid), spec(std::move(sp)) {}

    [[nodiscard]] std::string_view typeName() const noexcept override {
        return kTypeName;
    }

    void serialize(IEventSerializer& s) const override;

    [[nodiscard]] static SketchConstraintAdded*
        deserializeFrom(IEventDeserializer& d);
};

}  // namespace mycad::domain::sketch

// 在 .cpp 文件中：
// MYCAD_REGISTER_EVENT(SketchConstraintAdded)
```

**关键设计要点**：
- 所有字段 `const` → 编译期保证不可变
- 静态 `kTypeName` 字符串 → 序列化分发的唯一键
- `MYCAD_REGISTER_EVENT` 宏 → 派生事件零样板自动注册
- 注释文档"何时被发出 / 哪些读模型订阅" → 这是事件文档化的核心，比函数签名更重要

---

## 5.3 `EventStore` — 事件存储核心接口

**位置**：`src/domain/shared/include/mycad/domain/IEventStore.hpp`

**设计意图**：事件溯源的核心抽象。支持 append-only 写入、按聚合加载、订阅、快照。多种实现（内存、SQLite、PostgreSQL）。

```cpp
#pragma once

#include <chrono>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "mycad/domain/DomainEvent.hpp"
#include "mycad/domain/identity/AggregateId.hpp"
#include "mycad/domain/identity/EventId.hpp"

namespace mycad::domain {

// ---- 错误类型 ----
enum class EventStoreErrorKind {
    ConcurrencyConflict,    // expectedVersion 不匹配
    AggregateNotFound,
    SerializationFailed,
    StorageIoFailed,
    SnapshotCorrupted,
};

struct EventStoreError {
    EventStoreErrorKind kind;
    std::string         message;
    std::optional<Version> conflictingVersion;
};

template <typename T>
using EventStoreResult = std::expected<T, EventStoreError>;

// ---- 事件流（懒加载） ----
//
// 使用 C++20 ranges 设计：可以 stream-process 而不必把所有事件加载入内存
class EventStream {
public:
    using value_type = std::unique_ptr<DomainEvent>;

    class Iterator {
        // ... input iterator 实现，按需从存储读取
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::unique_ptr<DomainEvent>;
        using difference_type = std::ptrdiff_t;

        Iterator& operator++();
        value_type operator*();
        bool operator==(const Iterator&) const;
    };

    [[nodiscard]] Iterator begin();
    [[nodiscard]] Iterator end();

    [[nodiscard]] Version firstVersion() const noexcept;
    [[nodiscard]] Version lastVersion() const noexcept;
    [[nodiscard]] std::size_t estimatedCount() const noexcept;
};

// ---- 快照接口 ----
class ISnapshot {
public:
    virtual ~ISnapshot() = default;
    [[nodiscard]] virtual AggregateId aggregateId() const noexcept = 0;
    [[nodiscard]] virtual Version     version()     const noexcept = 0;
    virtual void serialize(IEventSerializer&) const = 0;
    [[nodiscard]] virtual std::string_view typeName() const noexcept = 0;
};

struct SnapshotRef {
    AggregateId aggregateId;
    Version     version;
    std::unique_ptr<ISnapshot> snapshot;
};

// ---- 事件订阅 ----
struct EventTypeFilter {
    // 空 = 订阅所有；否则只订阅指定类型
    std::vector<std::string> typeNames;

    [[nodiscard]] static EventTypeFilter all() { return {}; }
    [[nodiscard]] static EventTypeFilter only(std::initializer_list<std::string> types);
};

using EventHandler = std::function<void(const DomainEvent&)>;

// RAII 订阅句柄；析构时自动取消订阅
class SubscriptionHandle {
public:
    SubscriptionHandle() = default;
    SubscriptionHandle(const SubscriptionHandle&) = delete;
    SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;
    SubscriptionHandle(SubscriptionHandle&&) noexcept;
    SubscriptionHandle& operator=(SubscriptionHandle&&) noexcept;
    ~SubscriptionHandle();

private:
    friend class IEventStore;
    SubscriptionHandle(IEventStore* store, std::uint64_t id);
    IEventStore*  store_{nullptr};
    std::uint64_t id_{0};
};

// ---- 主接口 ----
class IEventStore {
public:
    virtual ~IEventStore() = default;

    // === 写入：原子追加多个事件，乐观并发检查 ===
    [[nodiscard]] virtual EventStoreResult<void>
        appendToStream(AggregateId aggId,
                       Version expectedVersion,
                       std::span<const DomainEvent* const> events) = 0;

    // === 读取：加载聚合事件流 ===
    [[nodiscard]] virtual EventStream
        loadStream(AggregateId aggId, Version fromVersion = Version{0}) = 0;

    // === 订阅：实时通知（用于读模型构建） ===
    [[nodiscard]] virtual SubscriptionHandle
        subscribe(EventTypeFilter filter, EventHandler handler) = 0;

    // === 全局事件流（用于读模型从零重建） ===
    [[nodiscard]] virtual EventStream
        loadAll(std::optional<std::chrono::system_clock::time_point> since = {}) = 0;

    // === 快照管理 ===
    [[nodiscard]] virtual EventStoreResult<void>
        saveSnapshot(std::unique_ptr<ISnapshot>) = 0;

    [[nodiscard]] virtual std::optional<SnapshotRef>
        loadLatestSnapshot(AggregateId aggId) = 0;

    // === 健康检查 / 统计 ===
    struct Stats {
        std::uint64_t totalEvents;
        std::uint64_t totalSnapshots;
        std::uint64_t storageBytes;
    };
    [[nodiscard]] virtual Stats stats() const = 0;
};

// ---- 工厂函数（按实现选择） ----
[[nodiscard]] std::unique_ptr<IEventStore> makeInMemoryEventStore();
[[nodiscard]] std::unique_ptr<IEventStore> makeSqliteEventStore(std::string_view path);

}  // namespace mycad::domain
```

**关键设计要点**：
- `appendToStream` 是事务边界 — 配套乐观并发检查（`expectedVersion`）
- `EventStream` 是 C++20 input range — 大事件流懒加载，不爆内存
- `SubscriptionHandle` RAII — 防止订阅泄漏
- `IEventStore` 是 Domain 层接口；具体实现（SQLite / Postgres）在 Infrastructure 层 — 严格 DIP

---

## 5.4 `IPlugin` — 插件接口基类

**位置**：`src/plugin/include/mycad/plugin/IPlugin.hpp`

**设计意图**：微内核插件总线的核心契约。生命周期清晰、能力可查询、ABI 兼容性可校验。

```cpp
#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mycad::plugin {

// ---- ABI 版本（Core C ABI，永远稳定） ----
inline constexpr std::uint32_t kCorePluginAbiVersion = 1;

// ---- 元数据 ----
struct SemVer {
    std::uint16_t major{0};
    std::uint16_t minor{0};
    std::uint16_t patch{0};
    std::string   prerelease;
    auto operator<=>(const SemVer&) const = default;
};

struct PluginDependency {
    std::string pluginId;
    SemVer minVersion;
    bool   optional{false};
};

struct PluginMetadata {
    std::string id;                              // 反向域名格式
    std::string displayName;
    SemVer      version;
    std::string author;
    std::string description;
    std::string homepageUrl;
    std::string licenseSpdx;                     // SPDX："LGPL-3.0-or-later"
    std::vector<PluginDependency> dependencies;
    std::uint32_t hostAbiVersionRequired{1};
};

// ---- 错误类型 ----
enum class PluginErrorKind {
    AbiMismatch,
    DependencyMissing,
    InitializationFailed,
    AlreadyActive,
    InvalidConfiguration,
};

struct PluginError {
    PluginErrorKind kind;
    std::string     message;
};

template <typename T>
using PluginResult = std::expected<T, PluginError>;

// ---- 前置声明 ----
class IPluginHost;
class ICommandProvider;
class IFormatHandler;
class IFeatureProvider;
class IUiContribution;
class ISystemProvider;

// ---- 插件接口 ----
class IPlugin {
public:
    virtual ~IPlugin() = default;

    [[nodiscard]] virtual PluginMetadata metadata() const = 0;

    // === 生命周期 ===
    [[nodiscard]] virtual PluginResult<void> onLoad(IPluginHost& host) = 0;
    [[nodiscard]] virtual PluginResult<void> onActivate() = 0;
    [[nodiscard]] virtual PluginResult<void> onDeactivate() = 0;
    virtual void onUnload() noexcept = 0;

    // === 能力查询 ===
    [[nodiscard]] virtual ICommandProvider*  asCommandProvider()  noexcept { return nullptr; }
    [[nodiscard]] virtual IFormatHandler*    asFormatHandler()    noexcept { return nullptr; }
    [[nodiscard]] virtual IFeatureProvider*  asFeatureProvider()  noexcept { return nullptr; }
    [[nodiscard]] virtual IUiContribution*   asUiContribution()   noexcept { return nullptr; }
    [[nodiscard]] virtual ISystemProvider*   asSystemProvider()   noexcept { return nullptr; }
};

}  // namespace mycad::plugin

// ---- 平台导出宏 ----
#if defined(_WIN32)
    #define MYCAD_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define MYCAD_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define MYCAD_PLUGIN_ENTRY(PluginClass)                                       \
    MYCAD_PLUGIN_EXPORT mycad::plugin::IPlugin* mycad_createPlugin() {        \
        return new PluginClass();                                             \
    }                                                                         \
    MYCAD_PLUGIN_EXPORT void mycad_destroyPlugin(mycad::plugin::IPlugin* p) { \
        delete p;                                                             \
    }                                                                         \
    MYCAD_PLUGIN_EXPORT std::uint32_t mycad_pluginAbiVersion() {              \
        return mycad::plugin::kCorePluginAbiVersion;                          \
    }
```

**关键设计要点**：
- 双 ABI 边界 — C 风格入口（永久稳定）+ C++ Host 接口（按主版本兼容）
- `asXxxProvider()` 模式 — 比 `dynamic_cast` 更明确、更稳定
- `onUnload` 标 `noexcept` — 卸载阶段不允许失败
- `MYCAD_PLUGIN_ENTRY` 宏 — 第三方插件作者一行代码搞定入口

---

## 5.5 `Sketch` — 草图聚合根

**位置**：`src/domain/sketch/include/mycad/domain/sketch/Sketch.hpp`

**设计意图**：DDD 聚合根的范式实现。业务方法返回事件而非直接 mutate，事件回放修改状态。

```cpp
#pragma once

#include <expected>
#include <memory>
#include <unordered_map>
#include <vector>

#include "mycad/domain/DomainEvent.hpp"
#include "mycad/domain/sketch/SketchId.hpp"
#include "mycad/domain/sketch/SketchEntity.hpp"
#include "mycad/domain/sketch/SketchConstraintSpec.hpp"
#include "mycad/domain/sketch/SketchStatus.hpp"
#include "mycad/domain/values/PlaneRef.hpp"
#include "mycad/domain/IConstraintSolver.hpp"

namespace mycad::domain::sketch {

enum class DomainErrorKind {
    InvalidArgument,
    EntityNotFound,
    ConstraintConflict,
    PlaneRefInvalid,
    SolverDiverged,
    OperationNotAllowedInState,
};

struct DomainError {
    DomainErrorKind kind;
    std::string     message;
};

template <typename T>
using DomainResult = std::expected<T, DomainError>;

using EventBatch = std::vector<std::unique_ptr<DomainEvent>>;

class Sketch {
public:
    [[nodiscard]] static DomainResult<EventBatch>
        create(SketchId id, PlaneRef plane,
               OperationId opId, UserId userId);

    Sketch() = default;
    void apply(const DomainEvent& event);

    // === 业务方法（不修改 this，返回事件）===
    [[nodiscard]] DomainResult<EventBatch>
        addLine(Point2D start, Point2D end,
                OperationId opId, UserId userId);

    [[nodiscard]] DomainResult<EventBatch>
        addCircle(Point2D center, double radius,
                  OperationId opId, UserId userId);

    [[nodiscard]] DomainResult<EventBatch>
        addArc(Point2D center, double radius, double startAngle, double endAngle,
               OperationId opId, UserId userId);

    [[nodiscard]] DomainResult<EventBatch>
        removeEntity(SketchEntityId entityId,
                     OperationId opId, UserId userId);

    [[nodiscard]] DomainResult<EventBatch>
        addConstraint(SketchConstraintSpec spec,
                      OperationId opId, UserId userId);

    [[nodiscard]] DomainResult<EventBatch>
        removeConstraint(SketchConstraintId constraintId,
                         OperationId opId, UserId userId);

    [[nodiscard]] DomainResult<EventBatch>
        solve(IConstraintSolver& solver,
              OperationId opId, UserId userId);

    // === 查询 ===
    [[nodiscard]] SketchId id() const noexcept { return id_; }
    [[nodiscard]] Version version() const noexcept { return version_; }
    [[nodiscard]] SketchStatus status() const noexcept { return status_; }
    [[nodiscard]] const PlaneRef& plane() const noexcept { return plane_; }

    [[nodiscard]] std::span<const SketchEntity> entities() const noexcept;
    [[nodiscard]] std::span<const SketchConstraintRef> constraints() const noexcept;

    [[nodiscard]] const SketchEntity* findEntity(SketchEntityId) const noexcept;

private:
    void onCreated(const SketchCreated&);
    void onEntityAdded(const SketchEntityAdded&);
    void onEntityRemoved(const SketchEntityRemoved&);
    void onConstraintAdded(const SketchConstraintAdded&);
    void onConstraintRemoved(const SketchConstraintRemoved&);
    void onSolved(const SketchSolved&);
    void onSolveFailed(const SketchSolveFailed&);

    SketchId              id_;
    PlaneRef              plane_;
    Version               version_{0};
    SketchStatus          status_{SketchStatus::Empty};
    std::vector<SketchEntity>              entities_;
    std::vector<SketchConstraintRef>       constraints_;
    std::unordered_map<SketchEntityId, std::size_t> entityIndex_;
    SketchEntityId        nextEntityId_{1};
    SketchConstraintId    nextConstraintId_{1};
};

}  // namespace mycad::domain::sketch
```

**关键设计要点**：
- **决策与执行分离**：业务方法只产出事件，不直接修改字段
- 默认构造允许 → 配合事件回放重建
- 业务方法签名包含 `OperationId opId, UserId userId` → 命令处理上下文显式传递
- `entityIndex_` → O(1) 查找加速
- 私有 `on*` 方法 → 每种事件单独的应用器，便于单测

---

## 5.6 `IEntityRegistry` — ECS 实体注册表接口

**位置**：`src/domain/shared/include/mycad/domain/IEntityRegistry.hpp`

**设计意图**：ECS 抽象层。隔离 EnTT 类型，保留未来替换可能。详见 [§九 §9.2.3](./09-self-host-strategy.md)。

```cpp
#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <span>
#include <type_traits>
#include <vector>

namespace mycad::domain::ecs {

struct EntityId {
    std::uint64_t value{0};
    [[nodiscard]] bool valid() const noexcept { return value != 0; }
    auto operator<=>(const EntityId&) const = default;
};

template <typename C>
concept Component =
    std::is_class_v<C> &&
    !std::is_polymorphic_v<C> &&
    std::is_move_constructible_v<C>;

template <typename C>
concept TagComponent = Component<C> && std::is_empty_v<C>;

template <Component... Cs>
class View {
public:
    class Iterator {
    public:
        using value_type = std::tuple<EntityId, Cs&...>;
        Iterator& operator++();
        value_type operator*();
        bool operator==(const Iterator&) const;
    };

    [[nodiscard]] Iterator begin();
    [[nodiscard]] Iterator end();
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    template <std::invocable<EntityId, Cs&...> Fn>
    void each(Fn&& fn);
};

class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void onConstructed(EntityId) = 0;
    virtual void onUpdated(EntityId)     = 0;
    virtual void onDestroyed(EntityId)   = 0;
};

class ObserverHandle { /* RAII */ };

class IEntityRegistry {
public:
    virtual ~IEntityRegistry() = default;

    // === 实体管理 ===
    [[nodiscard]] virtual EntityId create() = 0;

    template <Component... Cs>
    [[nodiscard]] EntityId createWith(Cs&&... cs) {
        EntityId e = create();
        (add(e, std::forward<Cs>(cs)), ...);
        return e;
    }

    virtual void destroy(EntityId) = 0;
    [[nodiscard]] virtual bool valid(EntityId) const noexcept = 0;
    [[nodiscard]] virtual std::size_t aliveCount() const noexcept = 0;

    // === Component ===
    template <Component C> C& add(EntityId, C);
    template <Component C> C& replace(EntityId, C);
    template <Component C> [[nodiscard]] C& get(EntityId);
    template <Component C> [[nodiscard]] const C& get(EntityId) const;
    template <Component C> [[nodiscard]] C* tryGet(EntityId) noexcept;
    template <Component C> [[nodiscard]] bool has(EntityId) const noexcept;
    template <Component C> void remove(EntityId);

    // === 查询 ===
    template <Component... Cs> [[nodiscard]] View<Cs...> view();

    // === Observer ===
    template <Component C>
    [[nodiscard]] ObserverHandle observe(std::shared_ptr<IObserver>);

    // === 类型注册（插件） ===
    template <Component C> void registerComponentType();

    // === 序列化 ===
    virtual void serializeAll(IEventSerializer&) const = 0;
    virtual void deserializeAll(IEventDeserializer&) = 0;

protected:
    using TypeId = std::uint64_t;
    template <typename C> static TypeId typeIdOf();

    virtual void* addImpl(EntityId, TypeId, void*, std::size_t) = 0;
    virtual void* getImpl(EntityId, TypeId) = 0;
    virtual bool  hasImpl(EntityId, TypeId) const noexcept = 0;
    virtual void  removeImpl(EntityId, TypeId) = 0;
};

[[nodiscard]] std::unique_ptr<IEntityRegistry> makeEnttRegistry();

}  // namespace mycad::domain::ecs
```

---

## 5.7 `CommandBus` — 命令总线（DDD 应用层）

**位置**：`src/application/include/mycad/application/CommandBus.hpp`

**设计意图**：DDD 应用层的入口。UI/插件通过 CommandBus 提交命令，CommandHandler 装载聚合 → 执行 → 持久化事件。

```cpp
#pragma once

#include <any>
#include <expected>
#include <functional>
#include <memory>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include "mycad/domain/IEventStore.hpp"
#include "mycad/domain/identity/UserId.hpp"
#include "mycad/domain/identity/OperationId.hpp"

namespace mycad::application {

template <typename C>
concept Command = std::is_class_v<C> && std::is_move_constructible_v<C>;

struct CommandContext {
    UserId       userId;
    OperationId  operationId;
    std::string  source;
    std::optional<std::string> traceId;
};

enum class CommandErrorKind {
    HandlerNotFound,
    AggregateLoadFailed,
    BusinessRuleViolated,
    ConcurrencyConflict,
    Vetoed,
    HandlerThrew,
};

struct CommandError {
    CommandErrorKind kind;
    std::string      message;
    std::any         detail;
};

template <typename T = void>
using CommandResult = std::expected<T, CommandError>;

template <Command C>
class ICommandHandler {
public:
    using ResultType = /* 从 handle 方法推导 */;
    virtual ~ICommandHandler() = default;
    virtual ResultType handle(const C&, const CommandContext&) = 0;
};

class ICommandInterceptor {
public:
    virtual ~ICommandInterceptor() = default;
    virtual bool beforeCommand(std::string_view, const std::any&,
                               const CommandContext&) = 0;
    virtual void afterCommand(std::string_view, const std::any&,
                              const CommandContext&, bool) = 0;
};

class CommandBus {
public:
    explicit CommandBus(std::shared_ptr<domain::IEventStore> store);
    ~CommandBus();

    template <Command C, typename Handler>
    void registerHandler(std::shared_ptr<Handler>);

    [[nodiscard]] auto registerInterceptor(std::shared_ptr<ICommandInterceptor>);

    template <Command C>
    auto send(C command, CommandContext ctx)
        -> CommandResult<typename ICommandHandler<C>::ResultType>;

    template <Command C>
    auto sendAsync(C command, CommandContext ctx)
        -> std::future<CommandResult<typename ICommandHandler<C>::ResultType>>;

private:
    struct HandlerEntry {
        std::shared_ptr<void> handler;
        std::function<std::any(const std::any&, const CommandContext&)> invoker;
    };

    std::shared_ptr<domain::IEventStore>            store_;
    std::unordered_map<std::type_index, HandlerEntry> handlers_;
    std::vector<std::weak_ptr<ICommandInterceptor>> interceptors_;
    mutable std::mutex                              mutex_;
};

}  // namespace mycad::application
```

**关键设计要点**：
- **每命令一 Handler** → DDD 原则：单一聚合负责单一命令类型
- **CommandContext 显式传递** → 比 thread_local 更可测试、协同友好
- **拦截器机制** → 插件可做审计/权限/否决，符合 Open-Closed
- **类型擦除内部，类型安全外部** → `send<C>` 的返回类型由 Handler 决定

---

## 5.8 文件位置总览

```
src/
├── domain/                          # ★ Domain 层：零外部依赖
│   ├── shared/include/mycad/domain/
│   │   ├── DomainEvent.hpp          # §5.2.1
│   │   ├── IEventStore.hpp          # §5.3
│   │   ├── IGeometryPort.hpp        # §5.1
│   │   ├── IConstraintSolver.hpp
│   │   └── IEntityRegistry.hpp      # §5.6
│   ├── sketch/include/mycad/domain/sketch/
│   │   ├── Sketch.hpp               # §5.5
│   │   └── events/
│   │       ├── SketchConstraintAdded.hpp   # §5.2.2
│   │       └── ...
│   ├── feature/
│   ├── assembly/
│   └── constraint/
├── application/include/mycad/application/
│   ├── CommandBus.hpp               # §5.7
│   └── handlers/
├── infrastructure/                  # ★ 具体实现
│   ├── geometry/OcctGeometryAdapter.{hpp,cpp}
│   ├── ecs/EnttRegistry.{hpp,cpp}
│   ├── solver/PlaneGcsSolver.{hpp,cpp}
│   ├── eventstore/{InMemory,Sqlite}EventStore.{hpp,cpp}
│   └── format/flatbuffers/
├── plugin/include/mycad/plugin/
│   ├── IPlugin.hpp                  # §5.4
│   ├── IPluginHost.hpp
│   └── PluginRegistry.{hpp,cpp}
└── ui/                              # Qt 实现
```

---

> **最后修订**：2026-05（首次拆分自 ARCHITECTURE.md）
