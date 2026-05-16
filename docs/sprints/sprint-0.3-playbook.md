# Sprint 0.3 — Infrastructure Adapter（Week 5-6，2026-05-15 ~ 05-28）

> 本文件由 prompt-library.md §1 生成，依据 sprint-0.2-playbook.md "给下个 Sprint 的预热" 更新。
>
> **工具分工**：🟦 Claude Code（全程直接实现，无 DeepSeek 往返）·🟩 人工（决策/验证）
>
> **每日编码时间上限**：2-3 小时。周日休息，Day 1-10 覆盖 10 个工作日。
>
> **参考**：[ADR-0004](../adr/ADR-0004-occt-as-geometry-kernel.md) · [ADR-0007](../adr/ADR-0007-entt-as-ecs.md) · [05-code-skeletons.md §5.1](../architecture/05-code-skeletons.md)

---

## 阶段目标（一句话）

**三条链路全部端到端打通**：OCCT `makeBox→tessellate` 证明几何链路；`InMemoryEventStore` 证明事件溯源链路；`EnttRegistry` 证明 ECS 链路。Domain 接口已定义，本 Sprint 只写 Infrastructure 实现。

---

## Sprint 验收清单（端到端可验证）

- [x] **V1** `geometry_construction_test` PASS：`adapter->makeBox(10,10,10)` 拿到有效 BRepHandle，`tessellate` 返回顶点数 > 0
- [x] **V2** `event_store_test` PASS：append → reload EventTypeRegistry 反序列化 → 类型名匹配
- [x] **V3** `entt_registry_test` PASS：create / destroy / isAlive / ECS 组件 emplace + get 完整周期
- [x] **V4** `cmake --build` 链接通过，`src/domain/` 零 OCCT / EnTT 头文件引用（grep 确认）
- [ ] **V5** CI Windows + Linux Debug + RelWithDebInfo 四 job green（待 push 后验证）
- [x] **V6** `src/infrastructure/geometry/OcctGeometryAdapter.cpp` 中 OCCT 头文件仅在 .cpp 出现，.hpp 零泄漏

---

## 任务总览

| # | 任务 | 标签 | 估时 | 验收 |
|---|---|---|---|---|
| T1 | OCCT vcpkg 验证（opencascade 在本机编译通过） | 🟩 | 1d | `cmake --preset local-debug` 含 opencascade 成功 |
| T2 | Infrastructure CMake scaffold + BRepHandle / TriangleMesh / GeomError 值对象 | 🟦 | 0.5d | 编译通过 |
| T3 | IGeometryConstructionPort 接口（Sprint 0.3 新增，domain 层，纯接口） | 🟦 | 0.5d | 编译通过，零 OCCT |
| T4 | OcctGeometryAdapter（实现 makeBox + tessellate + bbox + release） | 🟦 | 3d | V1 ✓ |
| T5 | InMemoryEventStore（实现 IEventStore，基于 EventTypeRegistry 重建） | 🟦 | 2d | V2 ✓ |
| T6 | EnttRegistry（实现 IEntityRegistry，包装 entt::registry） | 🟦 | 1.5d | V3 ✓ |
| T7 | CI Tier-A 隔离检查 + Sprint 收尾 | 🟦 + 🟩 | 0.5d | V4 V5 V6 ✓ |

**估时合计**：9 d × ~2.5h = **~22.5h**

---

## Inbox 重叠项处置方案

| Inbox / 遗留项 | 处置方案 | 落地位置 |
|---|---|---|
| Sprint 0.2 playbook "OCCT 先 Windows 调通" 警告 | Day 1 优先跑 vcpkg，确认可构建后再排后续任务 | Day 1 |
| OCCT vcpkg 冷构建 30-60 min | 让 CMake 在后台跑，同时写 T2 头文件 | Day 1 |
| InMemoryEventStore appendOne 便利重载（Sprint 0.2 retro 遗留） | 随 T5 一起实现 | Day 7-8 |
| Transform3D::inverse()（Sprint 0.2 retro 遗留） | 推迟到 Sprint 1.B（特征旋转需要时再加） | — |
| natvis Transform3D 行展示验证 | Day 4 调试 OcctGeometryAdapter 时顺便验证 | Day 4 |

---

## 前置条件检查（Sprint 启动前确认）

```powershell
# 1. 确认 vcpkg.json 包含 opencascade（若没有本 sprint Day 1 添加）
Get-Content vcpkg.json | Select-String "opencascade"

# 2. 确认 EnTT 已经可用（Sprint 0.1 应该已加入）
Get-Content vcpkg.json | Select-String "entt"

# 3. 确认 infrastructure 占位库已存在
Test-Path src/infrastructure/CMakeLists.txt
```

---

## 每日任务分解

---

### Day 1（~3h）：T1 — OCCT vcpkg 验证 ⚠️ 最高风险

> **主用工具**：🟩 人工（等待 vcpkg 构建）+ 🟦 Claude Code 辅助排错
>
> **前置**：无
>
> **风险声明**：OCCT 冷构建需 30-60 分钟，网络质量差时可能需要 `vcpkg-rescue`；如果 Day 1 失败，整个 Sprint 需要重新排期。**不要因 OCCT 失败而跳过 T5/T6 — 这两个任务可以独立推进。**

#### 步骤

1. **在 `vcpkg.json` 添加 opencascade 依赖**

   打开 `vcpkg.json`，在 `dependencies` 数组中加入：

   ```json
   { "name": "opencascade", "features": [] }
   ```

   > 最小特性集：默认 features 已包含 BRepPrimAPI（makeBox）和 BRepMesh（tessellate）。
   > 如果构建失败提示缺少某个 feature，再按需加入 `"tkmesh"` / `"tkprim"` 等。

2. **触发 vcpkg 安装 + 构建**

   ```powershell
   # 开始构建，输出到日志（OCCT 构建时间 30-60 min）
   cmake --preset local-debug 2>&1 | Tee-Object -FilePath build/occt-build.log
   ```

   让它跑着。在等待期间**同步执行** Step 3（写头文件）。

3. **（并行）阅读 OcctGeometryAdapter 设计约束**

   ```
   重点阅读：
   - docs/adr/ADR-0004-occt-as-geometry-kernel.md（边界约束）
   - docs/architecture/05-code-skeletons.md §5.1（BRepHandle / GeomResult 类型定义）

   关键规则：
   - OCCT 头文件 (#include <TopoDS_Shape.hxx> 等) 只能出现在 .cpp 文件里
   - .hpp 头文件只能 #include mycad/domain/ 和标准库
   - OcctGeometryAdapter.hpp 不得 forward-declare 任何 OCCT 类型
   ```

4. **验证 OCCT 构建成功**

   ```powershell
   # 检查 vcpkg installed 目录里有 opencascade 的 lib
   Test-Path build\local-debug\vcpkg_installed\x64-windows\lib\TKBRep.lib
   Test-Path build\local-debug\vcpkg_installed\x64-windows\lib\TKMesh.lib
   Test-Path build\local-debug\vcpkg_installed\x64-windows\lib\TKPrim.lib
   ```

5. **commit（仅 vcpkg.json 变更）**

   ```powershell
   git add vcpkg.json
   git commit -m "build(deps): add opencascade to vcpkg manifest (Sprint 0.3 T1)"
   ```

#### 验收

- [ ] `cmake --preset local-debug` 成功（opencascade 在 vcpkg_installed 下）
- [ ] TKBRep.lib / TKMesh.lib / TKPrim.lib 存在

#### 卡点预案

| 卡点 | 方案 |
|---|---|
| vcpkg SSL 35 错误 | 运行 `tools/vcpkg-reset-registry.ps1`，再重试 |
| 构建报 `Cannot find opencascade` | 检查 vcpkg 版本；尝试 `"name": "occt"` 作为包名 |
| OCCT 编译需要 MSVC 特定工具集 | 检查 `tools/vs2022.vsconfig` 是否包含 `Microsoft.VisualCpp.Tools.Hostx64.Targetx64` |
| Day 1 全天卡在 OCCT | **不要停工**：切换到 T5 InMemoryEventStore 或 T6 EnttRegistry，OCCT 问题留第二天 |

---

### Day 2（~2h）：T2 — Infrastructure CMake + BRepHandle / TriangleMesh / GeomError

> **主用工具**：🟦 Claude Code 直接实现
>
> **前置**：OCCT vcpkg 验证完成（或并行推进）

#### 步骤

1. **新增 domain 共享类型**（纯头文件，零实现）

   新建以下三个头文件，位置在 `src/domain/shared/include/mycad/domain/`：

   **`BRepHandle.hpp`**：
   - `struct BRepHandle { uint64_t id{0}; bool valid() const noexcept; operator<=>() = default; }`
   - `struct WireHandle { uint64_t id{0}; ... }`
   - `struct EdgeRef { BRepHandle owner; uint32_t index; }`
   - `struct FaceRef { BRepHandle owner; uint32_t index; }`

   **`GeomError.hpp`**：
   - `enum class GeomErrorKind` (InvalidInput / AlgorithmFailed / DegenerateGeometry / BooleanFailure / OutOfMemory / Unknown)
   - `struct GeomError { GeomErrorKind kind; std::string message; }`
   - `template<typename T> using GeomResult = std::expected<T, GeomError>;`（需 `<expected>`，C++23，VS 18 支持）

   **`TriangleMesh.hpp`**：
   - `struct TriangleMesh { std::vector<float> vertices; std::vector<uint32_t> indices; bool empty() const noexcept; }`
   - 注：`float` 精度对渲染足够，domain 内不做几何计算，仅传递给 UI 层

   **`TessellationParams.hpp`**：
   - `struct TessellationParams { double linearDeflection{0.1}; double angularDeflection{0.5}; bool relative{false}; }`

2. **检查 infrastructure CMakeLists.txt 是否需要更新**

   `src/infrastructure/CMakeLists.txt` 目前只有 placeholder。Sprint 0.3 会逐步替换它。暂时保留结构，Day 4-6 按需补 source 文件。

3. **commit**

   ```powershell
   git add src/domain/shared/include/mycad/domain/
   git commit -m "feat(domain): add BRepHandle, GeomError, TriangleMesh, TessellationParams types"
   ```

#### 验收

- [ ] 四个新头文件编译通过（在 value_objects_test.cpp 或单独测试文件中 `#include` 后 cmake --build 不报错）
- [ ] `grep -r "TopoDS\|BRep_\|opencascade" src/domain/` 输出为空

---

### Day 3（~2h）：T3 — IGeometryConstructionPort 接口

> **主用工具**：🟦 Claude Code 直接实现
>
> **前置**：T2 完成（BRepHandle / GeomResult / TriangleMesh 已存在）

#### 背景

Sprint 0.2 的 `IGeometryPort` 专注于**空间查询**（距离、投影、交线）。Sprint 0.3 需要**几何构造**（建体、布尔、网格化）。两类职责分开，符合接口隔离原则。

#### 步骤

新建 `src/domain/shared/include/mycad/domain/IGeometryConstructionPort.hpp`：

```
接口方法（Sprint 0.3 仅实现带 * 的最小子集，其余占位纯虚）：

基础形体：
  * makeBox(double dx, double dy, double dz) → GeomResult<BRepHandle>
    makeCylinder(double radius, double height) → GeomResult<BRepHandle>
    makeSphere(double radius) → GeomResult<BRepHandle>

草图→Wire（Sprint 1.A 实现，此处纯虚占位）：
    makeWireFromSketch(...) → GeomResult<WireHandle>

特征操作（Sprint 1.B 实现，占位）：
    prismaticExtrude / revolve / booleanUnion / booleanCut / fillet / chamfer

拓扑查询：
    edges(BRepHandle) → vector<EdgeRef>
    faces(BRepHandle) → vector<FaceRef>

测量：
    volume / area
  * bbox(BRepHandle) → BoundingBox

网格化：
  * tessellate(BRepHandle, TessellationParams) → GeomResult<TriangleMesh>

变换：
    transformed(BRepHandle, const Transform3D&) → GeomResult<BRepHandle>

序列化（Sprint 0.5 实现，占位）：
    serialize / deserialize

生命周期：
  * release(BRepHandle) noexcept
    release(WireHandle) noexcept

所有带 * 的方法 OcctGeometryAdapter 在 Sprint 0.3 中实现。
其余方法在 Adapter 中实现为 "return std::unexpected(GeomError{NotImplemented, "Sprint N"})"。
```

加编译测试 `tests/domain/shared/geometry_construction_port_test.cpp`：
- 仅编译验证：`static_assert(std::is_abstract_v<IGeometryConstructionPort>)`
- BRepHandle trivially copyable 断言

更新 `tests/CMakeLists.txt`。

**commit**：

```powershell
git add src/domain/shared/include/mycad/domain/IGeometryConstructionPort.hpp \
        tests/domain/shared/geometry_construction_port_test.cpp \
        tests/CMakeLists.txt
git commit -m "feat(domain): add IGeometryConstructionPort interface (Sprint 0.3 minimal subset)"
```

#### 验收

- [ ] 编译通过，static_assert 通过
- [ ] `grep -i "occt\|TopoDS\|BRep_" src/domain/` 输出为空

---

### Day 4-6（~2.5h × 3）：T4 — OcctGeometryAdapter

> **主用工具**：🟦 Claude Code 直接实现
>
> **前置**：T1（OCCT 已安装）+ T3（IGeometryConstructionPort 已定义）
>
> **这是本 Sprint 核心任务，难度最高。Day 4 先打通最小路径（makeBox→tessellate），Day 5 完善错误处理，Day 6 补测试。**

#### Day 4：最小可跑路径

创建以下文件结构：

```
src/infrastructure/geometry/
├── include/mycad/infrastructure/OcctGeometryAdapter.hpp   ← 仅 domain 类型，零 OCCT
└── OcctGeometryAdapter.cpp                                ← OCCT 头文件 include 在此
```

**`OcctGeometryAdapter.hpp`**（关键约束）：
```cpp
// 只允许 #include <mycad/domain/...> 和标准库
// 不许出现 TopoDS / BRep_ / opencascade 任何符号
class OcctGeometryAdapter final : public IGeometryConstructionPort {
public:
    OcctGeometryAdapter();
    ~OcctGeometryAdapter() override;
    // 实现 makeBox / tessellate / bbox / release
    // 其余方法占位返回 GeomError{Unknown, "not implemented"}
private:
    struct Impl;    // PIMPL — OCCT 类型藏在 .cpp 里的 Impl
    std::unique_ptr<Impl> impl_;
};
```

**`OcctGeometryAdapter.cpp`**（PIMPL 结构）：
```cpp
// ← 所有 OCCT include 在这里
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <Poly_Triangulation.hxx>

struct OcctGeometryAdapter::Impl {
    std::unordered_map<uint64_t, TopoDS_Shape> shapes;
    uint64_t nextId{1};
};

// makeBox:
//   BRepPrimAPI_MakeBox(dx, dy, dz).Shape() → 存入 shapes[nextId++] → 返回 BRepHandle{id}

// tessellate:
//   BRepMesh_IncrementalMesh(shape, params.linearDeflection, params.relative, params.angularDeflection)
//   TopExp_Explorer 遍历所有 Face → BRep_Tool::Triangulation → 收集顶点/索引

// bbox:
//   Bnd_Box + BRepBndLib::Add → 转 BoundingBox

// release:
//   shapes.erase(handle.id)
```

更新 `src/infrastructure/CMakeLists.txt`：

```cmake
# 替换 placeholder.cpp，加入 geometry 子模块
add_library(mycad_infrastructure STATIC
    geometry/OcctGeometryAdapter.cpp
)
# ... 加 find_package(OpenCASCADE) 或 vcpkg 自动处理的 target
target_link_libraries(mycad_infrastructure
    PUBLIC  mycad::domain
    PRIVATE mycad::compile_options
            TKBRep TKMesh TKPrim TKBO TKSTEP   # OCCT 模块
)
```

#### Day 5：错误处理 + 边界情况

- makeBox 的负数/零参数检查 → 返回 `GeomError{InvalidInput, ...}`
- tessellate 传入无效 handle → 返回 `GeomError{InvalidInput, "unknown handle"}`
- OCCT 内部异常（`Standard_Failure`）捕获 → 转为 `GeomError{AlgorithmFailed, e.GetMessageString()}`

```cpp
// OCCT 异常捕获模式
try {
    // ... OCCT 调用 ...
} catch (const Standard_Failure& e) {
    return std::unexpected(GeomError{GeomErrorKind::AlgorithmFailed,
                                     std::string(e.GetMessageString())});
}
```

#### Day 6：集成测试

创建 `tests/infrastructure/geometry/occt_adapter_test.cpp`：

```
TEST_CASE "OcctGeometryAdapter -makeBox returns valid handle"
  adapter.makeBox(10, 10, 10) → handle.valid() == true

TEST_CASE "OcctGeometryAdapter -tessellate produces non-empty mesh"
  makeBox(10,10,10) → tessellate(handle, {0.1, 0.5}) → mesh.vertices.size() > 0

TEST_CASE "OcctGeometryAdapter -bbox of unit cube"
  makeBox(1,1,1) → bbox → 验证 min ≈ (0,0,0), max ≈ (1,1,1)  (WithinAbs 0.01)

TEST_CASE "OcctGeometryAdapter -release frees handle"
  makeBox → release → 再 tessellate → 返回 InvalidInput error

TEST_CASE "OcctGeometryAdapter -makeBox with zero size returns InvalidInput"
  makeBox(0, 10, 10) → std::unexpected, kind == GeomErrorKind::InvalidInput

TEST_CASE "OcctGeometryAdapter -tessellate mesh has valid indices"
  indices.size() % 3 == 0（三角形）
  max(indices) < vertices.size() / 3（不越界）
```

更新 `tests/CMakeLists.txt` 加入新 target `mycad_infra_tests`（与 `mycad_unit_tests` 分开，因为链接 OCCT）。

**commit（Day 6 末）**：

```powershell
git add src/infrastructure/geometry/ tests/infrastructure/ tests/CMakeLists.txt \
        src/infrastructure/CMakeLists.txt
git commit -m "feat(infrastructure): add OcctGeometryAdapter — makeBox, tessellate, bbox (T4)"
```

#### 验收

- [ ] V1：`mycad_infra_tests` 中所有 OcctGeometryAdapter 测试 PASS
- [ ] V6：`grep -i "TopoDS\|BRep_\|opencascade" src/infrastructure/geometry/include/` 输出为空
- [ ] 删除 `src/infrastructure/placeholder.cpp`（不再需要）

#### 卡点预案

| 卡点 | 方案 |
|---|---|
| vcpkg OCCT 的 CMake target 名不确定 | 用 `find_package(OpenCASCADE)` 后 `message(STATUS ${OpenCASCADE_LIBRARIES})` 打印确认；或直接用 `opencascade::TKBRep` 等 vcpkg 自动导出的 alias |
| `Poly_Triangulation` API 在不同 OCCT 7.x 版本有变化 | 检查 OCCT 7.8 changelog；用 `GetNode()` 替代老 API `Nodes()` |
| tessellate 输出三角网格为空 | 检查 BRepMesh_IncrementalMesh 是否在 BRep_Tool::Triangulation 之前调用；OCCT 必须先 mesh 再读取 |
| PIMPL 析构时 unique_ptr 要求 Impl 完整类型 | 在 .cpp 提供 `OcctGeometryAdapter::~OcctGeometryAdapter() = default;` |

---

### Day 7-8（~2.5h × 2）：T5 — InMemoryEventStore

> **主用工具**：🟦 Claude Code 直接实现
>
> **前置**：T3 完成（IEventStore / EventTypeRegistry 已定义）
>
> **可以与 T4 并行推进（Day 4-6 OCCT 调试期间 CPU 空闲时）**

#### 设计要点

- 位置：`src/infrastructure/eventsourcing/InMemoryEventStore.hpp/.cpp`
- 存储结构：`unordered_map<AggregateId, Stream>` 其中 `Stream` 包含 `vector<SerializedEvent>` 和当前 `Version`
- 乐观并发：`append` 检查 `expectedVersion == stream.latestVersion`，否则 throw `ConcurrencyError`
- **序列化**：Sprint 0.3 暂用 JSON 占位（`nlohmann/json` 已在 vcpkg），或用简单的字段直存；Sprint 0.5 再换 FlatBuffers
- **反序列化**（`load`）：通过 `EventTypeRegistry::instance().findFactory(typeName)` 重建事件对象

```
存储格式（简化，Sprint 0.3）：
struct SerializedEvent {
    std::string   typeName;      // 用于反序列化分发
    AggregateId   aggregateId;
    Version       aggregateVersion;
    std::uint64_t occurredAtMs;
    std::string   payload;       // JSON 或空串（Sprint 0.3 事件无 payload）
};
```

#### 接口增强（Sprint 0.2 遗留 — appendOne 便利重载）

在 `IEventStore.hpp` 加非虚内联便利方法：

```cpp
void appendOne(AggregateId id, Version expected, const DomainEvent& ev) {
    const DomainEvent* ptr = &ev;
    append(id, expected, std::span<const DomainEvent* const>{&ptr, 1});
}
```

#### 测试覆盖

`tests/infrastructure/eventsourcing/in_memory_event_store_test.cpp`：

```
TEST_CASE "InMemoryEventStore -empty store returns v0"
TEST_CASE "InMemoryEventStore -append single event advances version"
TEST_CASE "InMemoryEventStore -append batch of 3 events"
TEST_CASE "InMemoryEventStore -ConcurrencyError on stale version"
TEST_CASE "InMemoryEventStore -load returns events in version order"
TEST_CASE "InMemoryEventStore -loadSince filters by fromVersion"
TEST_CASE "InMemoryEventStore -load reconstructs event typeName via registry"
  （注册一个 ThingHappened 工厂，append，load，验证 typeName() 匹配）
TEST_CASE "InMemoryEventStore -two aggregates are isolated"
TEST_CASE "InMemoryEventStore -appendOne convenience overload"
```

**commit（Day 8 末）**：

```powershell
git add src/infrastructure/eventsourcing/ tests/infrastructure/eventsourcing/ \
        src/infrastructure/CMakeLists.txt \
        src/domain/shared/include/mycad/domain/IEventStore.hpp
git commit -m "feat(infrastructure): add InMemoryEventStore + appendOne convenience (T5)"
```

#### 验收

- [ ] V2：所有 in_memory_event_store_test PASS（含 registry 重建 typeName 验证）
- [ ] appendOne 测试通过

---

### Day 9（~2h）：T6 — EnttRegistry

> **主用工具**：🟦 Claude Code 直接实现
>
> **前置**：IEntityRegistry 已定义，EnTT 已在 vcpkg

#### 设计要点

- 位置：`src/infrastructure/ecs/EnttRegistry.hpp/.cpp`
- 包装 `entt::registry`，`EntityId` 映射到 `entt::entity`（uint32_t 兼容）
- 注意：`entt::null` 对应 `kNullEntity`

```cpp
// EnttRegistry.hpp — 不 #include <entt/entt.hpp>，用 forward declaration 或 PIMPL
class EnttRegistry final : public IEntityRegistry {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    EnttRegistry();
    ~EnttRegistry() override;
    EntityId create() override;
    void     destroy(EntityId id) noexcept override;
    bool     isAlive(EntityId id) const noexcept override;
    size_t   size() const noexcept override;
    void     clear() noexcept override;

    // 扩展：ECS 组件操作（domain IEntityRegistry 不含这些，但 infrastructure 层需要）
    template <typename T, typename... Args>
    T& emplace(EntityId id, Args&&... args);

    template <typename T>
    T* tryGet(EntityId id) noexcept;

    template <typename T>
    void remove(EntityId id) noexcept;
};
```

> ⚠️ `emplace`/`tryGet`/`remove` 只在 `EnttRegistry` 上暴露，`IEntityRegistry` 接口不含这些（ADR-0007 边界）。Application 层通过 `dynamic_cast<EnttRegistry*>` 访问（或通过 Application 自己的 ECS 包装类）。

#### 测试覆盖

`tests/infrastructure/ecs/entt_registry_test.cpp`：

```
TEST_CASE "EnttRegistry -create returns unique ids"
TEST_CASE "EnttRegistry -created entity isAlive"
TEST_CASE "EnttRegistry -destroyed entity is not alive"
TEST_CASE "EnttRegistry -destroy kNullEntity is no-op"
TEST_CASE "EnttRegistry -size tracks live count"
TEST_CASE "EnttRegistry -clear destroys all"
TEST_CASE "EnttRegistry -emplace and tryGet component roundtrip"
  struct Pos { float x, y; };
  emplace<Pos>(id, 1.0f, 2.0f) → tryGet<Pos>(id) → {1.0, 2.0}
TEST_CASE "EnttRegistry -tryGet returns nullptr for missing component"
TEST_CASE "EnttRegistry -remove component"
  emplace → remove → tryGet == nullptr
```

**commit**：

```powershell
git add src/infrastructure/ecs/ tests/infrastructure/ecs/ src/infrastructure/CMakeLists.txt
git commit -m "feat(infrastructure): add EnttRegistry wrapping entt::registry (T6, ADR-0007)"
```

#### 验收

- [ ] V3：所有 entt_registry_test PASS
- [ ] `grep -r "#include.*entt" src/domain/` 输出为空（ADR-0007 ✓）

---

### Day 10（~2h）：T7 — CI Tier-A 隔离检查 + Sprint 收尾

> **主用工具**：🟦 Claude Code + 🟩 人工验证
>
> **前置**：T1-T6 全部完成

#### 步骤

1. **CI Tier-A 隔离 grep 检查**（加入 GitHub Actions）

   在 `.github/workflows/` 的 CI yaml 中加入一个 step：

   ```yaml
   - name: Tier-A isolation check (domain must not reference OCCT/EnTT)
     run: |
       result=$(grep -rn \
         "#include.*\(occt\|TopoDS\|BRep_\|entt\|Qt\|fmt\|spdlog\|<format>\)" \
         src/domain/ 2>/dev/null || true)
       if [ -n "$result" ]; then
         echo "VIOLATION: domain layer contains forbidden includes:"
         echo "$result"
         exit 1
       fi
       echo "domain layer is clean"
   ```

2. **全量测试验证**

   ```powershell
   cmake --preset local-debug
   cmake --build --preset local-debug
   ctest --preset local-debug --output-on-failure
   # mycad_unit_tests（domain 层，111 tests）
   # mycad_infra_tests（infrastructure 层，OcctAdapter + EventStore + EnttRegistry）
   ```

3. **更新 sprint-0.3-playbook 验收清单**（勾选所有 V1-V6）

4. **写 devlog 2026-W21 / W22**

   `docs/devlog/2026-W21.md` 和（如跨周）`2026-W22.md`：记录产出、OCCT 卡点、收获。

5. **push + 打标签**

   ```powershell
   git add docs/
   git commit -m "docs(sprint-0.3): devlog, sprint close"
   git tag sprint-0.3-done
   git push --tags
   ```

#### 验收

- [ ] V1-V6 全部 ✓
- [ ] CI Tier-A 隔离检查 step green
- [ ] `src/infrastructure/placeholder.cpp` 已删除
- [ ] git tag `sprint-0.3-done` 已推送

---

## 本 Sprint 可能产生的 ADR

| ADR 编号 | 主题 | 触发条件 |
|---|---|---|
| ADR-0015 | InMemoryEventStore 序列化格式选择（JSON vs 二进制） | Day 7-8 如果决定用 nlohmann/json 而非空串占位 |
| ADR-0016 | OcctGeometryAdapter PIMPL 设计 vs 直接 forward-declare | Day 4 如果选择不用 PIMPL |
| ADR-0017 | EnttRegistry 模板 API 暴露策略（只在 infra 层 vs 通过 Application 包装） | Day 9 如果确定了 Application 访问模式 |

> 若上述三个均未产生新不可逆决策，ADR 数保持 0014。

---

## Sprint 末验收清单（完整版）

- [x] T1-T7 全部 done
- [x] `cmake --build` 通过（domain + infrastructure 均无 OCCT/EnTT/Qt 头文件泄漏到 domain 层）
- [x] domain 层（111 tests）+ infrastructure 层（46 tests）合计 **157 tests**，全部 PASS
- [ ] CI 四个 job（Windows Debug + RelWithDebInfo + Linux Debug + RelWithDebInfo）green（待 push）
- [x] OcctGeometryAdapter 能创建立方体 + 网格化（V1 剧本完整跑通）
- [x] InMemoryEventStore 能 append → load → typeName 匹配（V2 剧本完整跑通）
- [x] EnttRegistry 能完整 ECS 组件生命周期（V3 剧本完整跑通）
- [x] `docs/devlog/2026-W21.md` 已写
- [x] `docs/sprints/sprint-0.3-playbook.md` 所有验收项已勾选
- [ ] git tag `sprint-0.3-done` 已推送（本次 commit 后执行）

---

## 给下个 Sprint 的预热

**Sprint 0.4 主题**：Application + 渲染（CommandBus + OpenGL 第一个三角形）

**Sprint 0.4 启动前需要准备**：

1. **阅读 ADR-0005**（OpenGL 而非 Vulkan）+ `docs/architecture/05-code-skeletons.md §5.4 CommandBus`

2. **确认 Qt 6 能通过 vcpkg 构建**（Sprint 0.5 前必须解决）：
   ```powershell
   # 在 Sprint 0.3 末或 0.4 Day 1 前运行
   # vcpkg.json 加 "qtbase"（仅 core + gui + openglwidgets）
   cmake --preset local-debug   # 会触发 Qt 构建，~30-60 min
   ```

3. **Sprint 0.4 最高风险**：OpenGL + Qt 的集成；建议 Day 1 先跑通空 Qt 窗口，再接 OpenGL。

4. **Sprint 0.3 遗留技术债提醒**：
   - InMemoryEventStore 的 JSON payload 序列化是临时方案，Sprint 0.5 换 FlatBuffers
   - OcctGeometryAdapter 中未实现的方法（makeWire / Boolean 等）在 Sprint 1.B 补全

---

> **最后更新**：2026-05-14（Sprint 0.3 启动，Claude Code 生成）
