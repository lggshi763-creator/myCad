# §二 技术架构设计

> 回到导航：[../../ARCHITECTURE.md](../../ARCHITECTURE.md)

## 2.1 整体架构融合哲学

### 2.1.1 为什么是"四合一"而不是"挑一种"

每种范式独立用都不够：

| 范式 | 单独使用的缺陷 | 在 myCad 中的不可替代价值 |
|---|---|---|
| **DDD 六边形** | 抽象层多、性能损耗、过度工程 | 把"业务概念"与"几何算法/UI/存储"彻底解耦，让 AI 辅助开发能基于稳定的领域语言而非易变的实现细节进行 |
| **事件溯源 + CQRS** | 读路径需重建、存储膨胀、心智复杂 | 是 CAD "无限 Undo + 协同 + 拓扑命名稳定性"的唯一架构级解法 |
| **ECS** | 不擅长表达层次关系、关系型查询难 | 几何对象数量动辄数十万，OO 继承层次会爆炸；ECS 的数据驱动 + 缓存友好对渲染/约束传播是数量级优势 |
| **微内核插件** | 接口成本高、跨边界调用慢 | CAM/CAE/BIM 的扩展时间跨度 5-10 年，没有插件总线就是定时炸弹 |

四者**职责正交**而非重叠 — 这是融合可行的根本前提。

### 2.1.2 各范式的职责边界（核心契约）

```
┌──────────────────────────────────────────────────────────────────┐
│                    Presentation Layer                             │
│              (Qt UI / Python Console / Web Frontend)              │
│                          ↓ Commands                               │
├──────────────────────────────────────────────────────────────────┤
│                    Application Layer (DDD)                        │
│   CommandBus → CommandHandler → load Aggregate → emit Events      │
│                          ↓ DomainEvents                           │
├──────────────────────────────────────────────────────────────────┤
│                       Domain Layer (DDD)                          │
│   Aggregates: Sketch / Feature / Assembly / ConstraintSystem      │
│   纯领域逻辑，不依赖任何 OCCT/Qt/OpenGL                            │
│                          ↓ Events                                 │
├──────────────────────────────────────────────────────────────────┤
│              Event Sourcing Infrastructure (CQRS Write Side)      │
│        EventStore (append-only) + Snapshot Manager                │
│                          ↓ Notify                                 │
├──────────────────────────────────────────────────────────────────┤
│                  ECS Read Model (CQRS Read Side)                  │
│   EntityRegistry + Components + Systems                           │
│   GeometryRebuildSystem 消费事件 → 调用 OCCT → 写入 Components    │
│                          ↓ Query                                  │
├──────────────────────────────────────────────────────────────────┤
│                       Render Layer                                │
│   RenderSystem 读 ECS Components → 提交 OpenGL DrawCall           │
└──────────────────────────────────────────────────────────────────┘

       ┌──────────────────────────────────────────────────┐
       │               Hexagonal Ports (横向切入)          │
       │   IGeometryPort → OCCT Adapter                   │
       │   IPersistencePort → SQLite/File Adapter         │
       │   IRenderPort → OpenGL Adapter                   │
       │   IPluginHost → 微内核 Bus                        │
       └──────────────────────────────────────────────────┘
```

**核心契约**（违反即架构崩塌）：

1. **Domain 层零依赖原则**：Domain 层只允许依赖 STL + Eigen（数学）。不允许 `#include` 任何 OCCT/Qt/OpenGL 头。详见 [ADR-0002](../adr/ADR-0002-domain-zero-deps.md)。
2. **Event 单向流**：Event 只能从 Domain 流向 EventStore 流向 ECS Read Model，不允许逆流。Read Model 不能修改 Domain。
3. **ECS 不持有领域语义**：ECS Component 只是"为渲染/查询优化的扁平数据"，不替代 Aggregate 作为业务真相。
4. **插件不能直接访问 Domain Aggregate**：插件通过 CommandBus 提交命令，通过订阅 Event 接收变化，永远不能拿到 Aggregate 的可变引用。

### 2.1.3 关键决策：写模型用 DDD，读模型用 ECS

```
                Command (用户操作)
                     │
                     ▼
              CommandHandler
                     │
                     ▼
         load Aggregate from EventStore
         (使用最近 Snapshot + replay 后续 Events)
                     │
                     ▼
            Aggregate.execute(Command)
            ↓ 校验业务规则、执行领域逻辑
            ↓ 产生 List<DomainEvent>
                     │
                     ▼
              EventStore.append(Events)
                     │
                     ├──────────────► Subscribers
                     │                    │
                     ▼                    ▼
            Snapshot Manager        ECS Read Model
            (周期性快照)              (实时更新 Components)
                                          │
                                          ▼
                                   RenderSystem 重绘
```

**为什么 Aggregate 用 OO（非 ECS），而 Component 用 ECS**：
- Aggregate 关心的是"业务规则一致性"，需要强类型 + 封装 + 领域方法。OO 在这里是最合适的工具。
- Component 关心的是"批量遍历 + 缓存命中率"，需要数据导向 + SoA 布局。ECS 在这里碾压 OO。
- 两者是 CQRS 的写/读模型，本来就该用不同的数据组织方式。

### 2.1.4 范式冲突点与化解方案

| 冲突点 | 表现 | 化解方案 |
|---|---|---|
| **DDD 强一致性 vs 事件最终一致性** | UI 刚发了命令，立刻读 ECS 还看不到结果 | 命令处理用同步事件分发（无消息队列），保证单进程内 ECS 在 CommandHandler 返回前已更新 |
| **事件溯源全量回放 vs CAD 重建慢** | 数千事件回放要 30 秒 | 强制快照策略：每 100 事件 + 用户 Save 时必快照 |
| **ECS 扁平 vs DDD 层次结构** | 装配体的零件父子关系如何表达 | 用 `ParentEntityComponent {EntityId}` + 按需 join，避免做"嵌套 Component" |
| **微内核插件 vs DDD 边界完整性** | 插件可以破坏聚合不变式 | 插件只能通过 CommandBus + 事件订阅，无法获取 Aggregate 引用 |

---

## 2.2 DDD 领域建模设计

### 2.2.1 限界上下文总览

```
┌────────────┐ "草图修改了"   ┌────────────┐
│  Sketch BC │─────────────▶ │ Feature BC │
└────────────┘                └─────┬──────┘
      ▲                             │
      │                             │ "特征结果几何就绪"
      │ "约束求解需要"               ▼
┌─────┴──────┐                ┌────────────┐
│Constraint  │◀─────────────  │Assembly BC │
│    BC      │ "装配约束求解"  └────────────┘
└────────────┘
```

**上下文映射关系**：
- **Sketch BC ↔ Feature BC**：上游/下游，Feature 依赖 Sketch 的几何结果
- **Sketch BC ↔ Constraint BC**：合作关系，Sketch 持有约束系统但求解算法委托给 Constraint BC
- **Feature BC ↔ Assembly BC**：上游/下游，Assembly 引用 Feature 的零件几何
- **Assembly BC ↔ Constraint BC**：客户/供应商，Assembly 使用 Constraint BC 的求解器

### 2.2.2 Sketch BC（草图上下文）

#### 聚合根：`Sketch`

```cpp
class Sketch {
    SketchId id_;
    PlaneRef plane_;
    std::vector<SketchEntity> entities_;
    std::vector<SketchConstraintRef> constraints_;
    SketchStatus status_;
    Version version_;

public:
    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        addLine(Point2D start, Point2D end);

    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        addConstraint(SketchConstraintSpec spec);

    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        solve(IConstraintSolver& solver);

    void apply(const DomainEvent& event);
};
```

完整骨架见 [§五 §5.5](./05-code-skeletons.md#55-sketch--草图聚合根)。

#### 核心实体与值对象

| 类型 | 角色 | 字段 |
|---|---|---|
| `Sketch` | 聚合根 | id, plane, entities, constraints, status |
| `SketchEntity` | 实体 | id, kind{Line/Circle/Arc/...}, geometry data |
| `Point2D` | 值对象 | x, y（不可变） |
| `PlaneRef` | 值对象 | featureId, faceIndex |
| `SketchConstraintSpec` | 值对象 | type, entityRefs |
| `SketchStatus` | 值对象（枚举） | OK / UnderConstrained / OverConstrained / Failed |

#### 领域事件

```cpp
struct SketchCreated : DomainEvent { SketchId id; PlaneRef plane; };
struct SketchEntityAdded : DomainEvent { SketchId sketchId; SketchEntity entity; };
struct SketchEntityRemoved : DomainEvent { SketchId sketchId; SketchEntityId entityId; };
struct SketchConstraintAdded : DomainEvent { SketchId sketchId; SketchConstraintSpec spec; };
struct SketchSolved : DomainEvent { SketchId sketchId; std::vector<EntityCoordUpdate> updates; };
struct SketchSolveFailed : DomainEvent { SketchId sketchId; SolverDiagnostics diag; };
```

**为什么这样设计**：
- **事件命名为过去时**：强制思维转变 — 你不是在"做什么"，而是在"记录已经发生了什么"
- **事件不可变**：所有字段 `const`，避免历史被篡改（详见 [ADR-0003](../adr/ADR-0003-events-immutable.md)）
- **聚合方法返回 events 而非直接 mutate**：让 CommandHandler 控制何时持久化

### 2.2.3 Feature BC（特征上下文）

#### 聚合根：`FeatureTree`

整个文档的特征树作为一个聚合根（不是每个 Feature 独立做聚合），原因：
- 特征之间存在强依赖（Pad2 依赖 Pad1 的面），跨聚合事务难处理
- 重建需要按拓扑序遍历，统一聚合管理更简单
- 单文档量级（百级特征）不会超出单聚合容量

```cpp
class FeatureTree {
    FeatureTreeId id_;
    std::vector<FeatureNode> nodes_;
    std::map<FeatureId, FeatureNode*> index_;
    Version version_;

public:
    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        addPadFeature(SketchRef sketch, double depth);

    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        suppressFeature(FeatureId id);

    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        rebuild(IGeometryPort& geom);

    void apply(const DomainEvent& event);
};
```

#### 核心实体与值对象

- `FeatureNode`（实体）：id, kind, parameters, dependencies, status
- `FeatureKind`（枚举）：Pad / Pocket / Revolve / Fillet / Chamfer / Mirror / Pattern / ...
- `FeatureParameters`（值对象，多态）：每种特征对应一种参数结构（`std::variant`）
- `BRepHandle`（值对象）：对 OCCT TopoDS_Shape 的不透明引用

### 2.2.4 Assembly BC（装配上下文）

#### 聚合根：`Assembly`

```cpp
class Assembly {
    AssemblyId id_;
    std::vector<ComponentInstance> instances_;
    std::vector<AssemblyConstraintRef> constraints_;
    Transform3D worldTransform_;
    Version version_;

public:
    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        insertComponent(DocumentRef partRef, Transform3D initialTransform);

    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        addMateConstraint(MateSpec spec);

    [[nodiscard]] std::expected<std::vector<DomainEvent>, DomainError>
        solveAssembly(IConstraintSolver& solver);
};
```

#### 关键设计

- `ComponentInstance` 通过 `DocumentRef` 指向另一个 myCad 文档，不直接持有零件几何
- 装配约束独立于草图约束 → 复用同一个 Constraint BC 的求解器
- 装配树是树状（不是 DAG），子装配体作为整体引用

### 2.2.5 Constraint BC（约束上下文）

#### 聚合根：`ConstraintSystem`

```cpp
class ConstraintSystem {
    ConstraintSystemId id_;
    std::vector<Variable> variables_;
    std::vector<Constraint> constraints_;
    SolverConfig config_;

public:
    [[nodiscard]] std::expected<SolveResult, SolverError>
        solve(const InitialGuess& guess);

    DiagnosticReport diagnose() const;
};
```

#### 关键设计

- **Constraint BC 是"无状态计算服务"**：不持有几何，只做数学求解
- **被多个上下文复用**：Sketch BC 用它解 2D 约束，Assembly BC 用它解 3D 装配约束
- **领域事件少**：求解事件由调用方负责发出

### 2.2.6 上下文之间的事件流

```
SketchSolved ──────► Feature BC（订阅，触发依赖该草图的特征重建）
                  └► ECS Read Model（更新草图几何 Component）

FeatureRebuilt ────► Assembly BC（订阅，触发装配重新求解）
                  └► ECS Read Model（更新零件几何 Component）

AssemblySolved ────► ECS Read Model（更新世界变换 Component）
```

**为什么用领域事件而非直接调用**：
- 上下文之间通过事件解耦，未来可以独立进程化（远期协同/云端架构基础）
- 事件总线可以做拦截（日志/审计/AI 训练数据采集）

---

## 2.3 事件溯源与 CQRS 设计

### 2.3.1 EventStore 数据结构

```cpp
class DomainEvent {
public:
    EventId id;                  // ULID
    AggregateId aggregateId;
    AggregateType aggregateType;
    Version expectedVersion;     // 乐观并发控制
    std::chrono::system_clock::time_point timestamp;
    UserId userId;
    LogicalClock vectorClock;    // 协同准备：CRDT-friendly

    virtual EventTypeName typeName() const = 0;
    virtual void serialize(Serializer&) const = 0;
    virtual ~DomainEvent() = default;
};

class IEventStore {
public:
    virtual std::expected<void, ConcurrencyError>
        append(AggregateId aggId, Version expected,
               std::span<const DomainEvent*> events) = 0;

    virtual EventStream loadStream(AggregateId aggId,
                                    Version fromVersion = Version{0}) = 0;

    virtual SubscriptionHandle subscribe(EventTypeFilter filter,
                                         EventHandler handler) = 0;

    virtual void saveSnapshot(AggregateId aggId, Version v,
                              std::unique_ptr<Snapshot> snap) = 0;
    virtual std::optional<SnapshotRef> loadLatestSnapshot(AggregateId aggId) = 0;
};
```

完整接口见 [§五 §5.3](./05-code-skeletons.md#53-eventstore--事件存储核心接口)。

#### 物理存储

| 实现 | 适用阶段 | 优点 | 缺点 |
|---|---|---|---|
| **In-Memory EventStore** | Phase 0 单测 | 极快、易测试 | 不持久化 |
| **SQLite EventStore** | Phase 1 单文件 | 单文件 = 单 .mycad 文档 | 写入性能上限较低 |
| **RocksDB EventStore** | Phase 2 大型项目 | 高写入吞吐 | 嵌入复杂度高 |
| **PostgreSQL EventStore** | Phase 3 协同/企业 | 多用户、事务、JSON 查询 | 部署门槛 |

**推荐路径**：In-Memory（Day 1）→ SQLite（Phase 1，与 .mycad 格式合体）→ 企业版按需引入 PostgreSQL。

### 2.3.2 SQLite Schema 设计

```sql
CREATE TABLE events (
    event_id        TEXT PRIMARY KEY,        -- ULID
    aggregate_id    TEXT NOT NULL,
    aggregate_type  TEXT NOT NULL,
    version         INTEGER NOT NULL,
    event_type      TEXT NOT NULL,
    payload         BLOB NOT NULL,            -- FlatBuffers
    metadata        BLOB,
    created_at      INTEGER NOT NULL,
    UNIQUE (aggregate_id, version)            -- 乐观并发的核心约束
);

CREATE INDEX idx_events_aggregate ON events(aggregate_id, version);
CREATE INDEX idx_events_type ON events(event_type);
CREATE INDEX idx_events_time ON events(created_at);

CREATE TABLE snapshots (
    aggregate_id    TEXT NOT NULL,
    version         INTEGER NOT NULL,
    payload         BLOB NOT NULL,
    created_at      INTEGER NOT NULL,
    PRIMARY KEY (aggregate_id, version)
);

CREATE TABLE document_meta (
    key             TEXT PRIMARY KEY,
    value           TEXT NOT NULL
);
```

**为什么这样设计**：
- `(aggregate_id, version) UNIQUE` 是乐观并发的根基 — 两个并发命令最多一个能写入成功
- payload 用 FlatBuffers 而非 JSON：CAD 事件量大（百万级），二进制反序列化零拷贝是必要的
- 事件 ID 用 ULID（不是 UUID v4）：ULID 时间排序、25% 短、与 SQLite B+Tree 索引友好

### 2.3.3 快照策略

#### 何时打快照

```cpp
class SnapshotPolicy {
public:
    bool shouldSnapshot(AggregateId aggId,
                        Version currentVersion,
                        Version lastSnapshotVersion,
                        TriggerContext ctx) const {
        if (currentVersion - lastSnapshotVersion >= 100) return true;
        if (ctx == TriggerContext::UserSave) return true;
        if (ctx == TriggerContext::ExpensiveOpCompleted) return true;
        if (ctx == TriggerContext::PeriodicCheckpoint) return true;
        return false;
    }
};
```

#### 快照内容

不是聚合的完整内存状态，而是"重建聚合所需的最小状态 + 关联的几何缓存"：

```cpp
struct FeatureTreeSnapshot {
    Version version;
    std::vector<FeatureNodeSnapshot> nodes;
    std::map<FeatureId, BRepBinaryBlob> cachedGeometry;
};
```

**为什么缓存几何**：OCCT 的布尔运算可能耗时几秒，回放完事件后还要重新算几何 = 双倍开销。快照里直接带上 BRep 的二进制序列化（OCCT 提供 `BinTools` API）。

#### 快照序列化

- 格式：FlatBuffers（与事件一致）
- BRep 几何：OCCT `BinTools::Write/Read` 直接序列化为二进制 blob
- 压缩：Zstandard（CAD 数据有大量结构化重复，压缩率 5-10x）

### 2.3.4 写模型与读模型分离

```
       ┌─────────────────────┐
       │ User Action (UI)    │
       └──────────┬──────────┘
                  ▼
       ┌─────────────────────┐
       │   CommandBus        │
       └──────────┬──────────┘
                  ▼
       ┌─────────────────────┐
       │  CommandHandler     │
       │  ┌───────────────┐  │
       │  │ Load Aggregate│  │ ← Snapshot + Replay
       │  │ Execute       │  │
       │  │ Emit Events   │  │
       │  └───────────────┘  │
       └──────────┬──────────┘
                  │
                  ▼
       ┌─────────────────────┐
       │   EventStore        │ ◄── Write Model 终点
       │  (append-only)      │
       └──────────┬──────────┘
                  │
        ┌─────────┼─────────┬──────────────┐
        ▼         ▼         ▼              ▼
  ┌──────────┐ ┌──────┐ ┌──────────┐ ┌────────────┐
  │ECS Read  │ │ AI   │ │ Render   │ │ Plugin     │
  │ Model    │ │Model │ │  State   │ │ Subscribers│
  └──────────┘ └──────┘ └──────────┘ └────────────┘
```

#### 读模型类型与刷新策略

| 读模型 | 用途 | 刷新策略 |
|---|---|---|
| **ECS Read Model** | 渲染、选择、交互 | 同步刷新 |
| **特征树 UI Model** | 左侧树状面板 | 同步刷新 |
| **AI 上下文 Model** | LLM 输入序列化 | 按需构建 |
| **撤销栈 UI Model** | 撤销重做面板 | 直接读 EventStore |

**为什么读模型多个**：每个 UI 场景对数据形状的需求不同。强行统一会出现"读模型臃肿+查询路径低效"的反模式。CQRS 的精髓就是允许多个读模型为不同查询模式优化。

### 2.3.5 Undo/Redo 混合策略

#### 单纯事件回放方案的缺陷

```
做了 50 步操作，撤销 1 步需要：
  从最近 Snapshot 加载 → 回放前 49 个事件 → 重建几何
  → 可能需要数十次 OCCT 调用 → 数秒延迟
```

#### 混合方案：Snapshot 池 + 反向事件

```cpp
class UndoRedoManager {
    EventStore& store_;
    std::deque<OperationCheckpoint> recentCheckpoints_;
    static constexpr size_t kMaxCheckpoints = 20;

public:
    void recordOperation(const std::vector<EventId>& opEvents,
                         std::shared_ptr<AggregateSnapshot> beforeSnap,
                         std::shared_ptr<AggregateSnapshot> afterSnap) {
        recentCheckpoints_.push_back({opEvents, beforeSnap, afterSnap});
        if (recentCheckpoints_.size() > kMaxCheckpoints)
            recentCheckpoints_.pop_front();
    }

    void undo() {
        if (recentCheckpoints_.empty()) {
            slowPathUndo();
            return;
        }
        auto& last = recentCheckpoints_.back();
        restoreFromSnapshot(last.beforeSnap);
        recentCheckpoints_.pop_back();
        markUndone(last.opEvents);
    }
};
```

**为什么不删除事件**：事件不可变是事件溯源的基石。撤销 = 标记 + 反向操作，永远不能 DELETE。这保证了完整审计轨迹。

#### 撤销栈的"操作"粒度

一个用户操作（如"添加倒角"）可能产生多个事件。撤销时必须以"操作"为粒度：

```cpp
class OperationScope {
    OperationId opId_;
public:
    OperationScope() : opId_(generateOpId()) {}
    OperationId id() const { return opId_; }
};

struct DomainEvent {
    OperationId operationId;  // 同一用户操作的所有事件共享此 ID
};
```

### 2.3.6 协同编辑的事件偏序设计

#### 目标：未来支持 2-10 人实时协同

#### 关键挑战

CAD 协同比文档协同难得多 — 几何操作不可交换：
- 文档：A 在第 5 行插入"hello"，B 在第 5 行插入"world" → 容易合并
- CAD：A 对 Pad1 倒角，B 删除 Pad1 → 怎么合？

#### 设计决策：分两层处理

**Layer 1（不可交换的结构操作）**：用集中式权威服务器仲裁
- 添加/删除 Feature 必须走服务器分配 sequence number
- 服务器拒绝冲突操作

**Layer 2（可交换的内容操作）**：用 CRDT
- 草图内拖拽点的位置：用 LWW 或更复杂的 CRDT
- 每个事件携带 vectorClock

```cpp
struct LogicalClock {
    std::map<UserId, uint64_t> counters;

    bool happensBefore(const LogicalClock& other) const;
    bool concurrent(const LogicalClock& other) const;
    LogicalClock merge(const LogicalClock& other) const;
};

struct DomainEvent {
    LogicalClock vectorClock;
    UserId originator;
};
```

**为什么 Phase 0 就预留 vectorClock 字段**：协同是 Phase 3 才做，但事件格式一旦定型就是契约。后期加字段会污染历史事件，不如一开始就预留。

---

## 2.4 ECS 几何实体系统设计

### 2.4.1 EnTT vs 自研

| 维度 | EnTT | 自研 ECS |
|---|---|---|
| **代码量** | 0 行（接入即用） | 5k-10k 行（含测试） |
| **性能** | C++ 圈最快之一 | 不可能更快 |
| **特性** | 完整 | 自己实现 |
| **学习成本** | 中（API 庞大） | 低（你设计的） |
| **依赖风险** | 单维护者 + 头文件库 | 无 |
| **定制空间** | 受限 | 完全自由 |

**决策：使用 EnTT**（详见 [ADR-0007](../adr/ADR-0007-entt-as-ecs.md)）

**为什么**：
- 个人开发者的核心约束是时间，自研 ECS 会消耗 1-2 个月而无差异化价值
- EnTT 是头文件库，集成零成本（vcpkg 一行）
- EnTT 的 sparse_set 设计是当前 ECS 实现的 SOTA
- 通过 Adapter 层封装 EnTT API，保留未来替换可能（详见 [§九 §9.2.3](./09-self-host-strategy.md)）

**抽象边界**：

```cpp
class IEntityRegistry {
public:
    virtual EntityId create() = 0;
    virtual void destroy(EntityId) = 0;
    template <typename C> C& add(EntityId, C component);
    template <typename C> C* tryGet(EntityId);
    template <typename... Cs> auto view();
};

class EnttRegistry : public IEntityRegistry {
    entt::registry registry_;
};
```

完整接口见 [§五 §5.6](./05-code-skeletons.md#56-ientityregistry--ecs-实体注册表接口)。

### 2.4.2 核心 Component 类型

#### 几何相关

```cpp
struct DomainLinkComponent {
    AggregateType type;
    AggregateId aggregateId;
    Version aggregateVersion;
};

struct BRepGeometryComponent {
    BRepHandle handle;
    BoundingBox bbox;
    bool isVisible = true;
};

struct MeshComponent {
    std::shared_ptr<TriangleMesh> mesh;
    bool dirty = false;
    LODLevel currentLod = LODLevel::Auto;
};

struct Sketch2DEntityComponent {
    SketchId parent;
    SketchEntityKind kind;
    std::vector<Point2D> controlPoints;
};

struct TransformComponent {
    Eigen::Isometry3d localTransform;
    Eigen::Isometry3d worldTransform;
    bool dirty = true;
};

struct ParentComponent { EntityId parent; };
struct ChildrenComponent { std::vector<EntityId> children; };
```

#### 约束相关

```cpp
struct ConstraintComponent {
    ConstraintId id;
    std::vector<EntityId> participants;
    ConstraintKind kind;
    ConstraintParameters params;
    bool satisfied = false;
};

struct UnderConstrainedTagComponent {};
struct OverConstrainedTagComponent {};
```

#### 渲染相关

```cpp
struct MaterialComponent {
    Color baseColor;
    float metallic;
    float roughness;
    std::optional<TextureId> albedoTexture;
};

struct RenderableComponent {
    RenderPipelineId pipeline;
    RenderLayer layer;
    bool castShadow = true;
};

struct SelectionComponent {
    SelectionState state;
};

struct GpuResourceComponent {
    BufferHandle vbo;
    BufferHandle ibo;
    bool needsUpload = true;
};
```

#### 标签 Component（零字节，用于过滤）

```cpp
struct VisibleTag {};
struct PickableTag {};
struct AssemblyRootTag {};
struct DirtyGeometryTag {};
```

### 2.4.3 核心 System 列表

| System | 输入 Component | 输出 Component | 职责 |
|---|---|---|---|
| `EventConsumerSystem` | — | `DomainLinkComponent`, `DirtyGeometryTag` | 订阅 EventStore，将事件映射为 ECS 状态变化 |
| `GeometryRebuildSystem` | `DirtyGeometryTag` | `BRepGeometryComponent` | 调用 OCCT 重算几何 |
| `TessellationSystem` | `BRepGeometryComponent` | `MeshComponent` | BRep → Triangle Mesh |
| `TransformSystem` | `TransformComponent`, `ParentComponent` | `TransformComponent` (worldTransform) | 计算世界变换 |
| `BoundingBoxSystem` | `BRepGeometryComponent` | `BoundingBox` | 计算 AABB |
| `CullingSystem` | `BoundingBox`, `TransformComponent` | `VisibleTag` | 视锥体剔除 |
| `LODSystem` | `MeshComponent`, distance | `MeshComponent` (currentLod) | LOD 选择 |
| `ConstraintPropagationSystem` | `ConstraintComponent`, `Sketch2DEntityComponent` | (events) | 约束变化触发求解 |
| `GpuUploadSystem` | `MeshComponent`, `dirty` | `GpuResourceComponent` | 上传到 GPU |
| `RenderSystem` | `VisibleTag`, `GpuResourceComponent`, `MaterialComponent` | DrawCall | 提交渲染命令 |
| `SelectionSystem` | (mouse pick) | `SelectionComponent` | 处理选择 |
| `PluginSystem` | (插件配置) | (任意) | 让插件可以注册自定义 System |

### 2.4.4 ECS ↔ DDD 的映射

#### 一个聚合可能对应多个 ECS Entity

例如 `FeatureTree` 聚合包含 N 个 Feature → ECS 中有 N 个 Entity，每个通过 `DomainLinkComponent` 指回 `(FeatureTree, FeatureId)`。

#### 事件驱动的同步

```cpp
class EventConsumerSystem {
    IEntityRegistry& ecs_;
    std::unordered_map<AggregateId, std::vector<EntityId>> aggToEntities_;

public:
    void onFeatureAdded(const FeatureAdded& evt) {
        EntityId eid = ecs_.create();
        ecs_.add(eid, DomainLinkComponent{
            .type = AggregateType::FeatureTree,
            .aggregateId = evt.treeId,
            .aggregateVersion = evt.aggregateVersion
        });
        ecs_.add(eid, DirtyGeometryTag{});
        aggToEntities_[evt.treeId].push_back(eid);
    }

    void onFeatureRebuilt(const FeatureRebuilt& evt) {
        auto eid = findEntity(evt.featureId);
        auto& brep = ecs_.add<BRepGeometryComponent>(eid, {evt.resultHandle, ...});
        ecs_.remove<DirtyGeometryTag>(eid);
    }
};
```

**为什么这种"事件→ECS 单向同步"模式**：
- ECS 永远不会错（它是 EventStore 的纯函数派生），出问题就丢弃 ECS 全部重建
- 如果 ECS 直接被 UI 修改 → 修改不会进入 EventStore → 重启就丢失 = 灾难
- 强制单向流让"真相"始终在 EventStore 中，符合 CQRS 原则

### 2.4.5 响应式约束传播网络

#### 问题

用户拖拽草图中的一个点 A，与 A 相关的所有约束需要重新求解，可能波及全图。

#### 设计：ECS 标签 + System 调度

```cpp
struct PointMoved : DomainEvent { SketchEntityId entity; Point2D newPos; };

void onPointMoved(const PointMoved& evt) {
    auto eid = findEntity(evt.entity);
    auto* sketchComp = ecs_.tryGet<Sketch2DEntityComponent>(eid);
    sketchComp->controlPoints[0] = evt.newPos;

    for (auto constraintEid : queryConstraintsInvolving(eid)) {
        ecs_.add<ConstraintDirtyTag>(constraintEid);
    }
}

void ConstraintPropagationSystem::update() {
    auto dirtyConstraints = ecs_.view<ConstraintComponent, ConstraintDirtyTag>();
    if (dirtyConstraints.empty()) return;

    auto affectedSketches = collectSketches(dirtyConstraints);

    for (auto sketchId : affectedSketches) {
        commandBus_.send(SolveSketchCommand{sketchId});
    }

    for (auto eid : dirtyConstraints) ecs_.remove<ConstraintDirtyTag>(eid);
}
```

**为什么用"脏标签 + 帧末批处理"而非每次操作都立即求解**：
- 用户连续拖拽 60 fps，每次都求解会卡顿
- 批处理在帧末统一求解，等同于"防抖"
- 标签法天然适配 ECS 的批量处理模式

---

## 2.5 微内核插件总线设计

### 2.5.1 IPlugin 接口

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual PluginMetadata metadata() const = 0;

    virtual std::expected<void, PluginError> onLoad(IPluginHost& host) = 0;
    virtual std::expected<void, PluginError> onActivate() = 0;
    virtual std::expected<void, PluginError> onDeactivate() = 0;
    virtual void onUnload() noexcept = 0;

    virtual ICommandProvider* asCommandProvider() { return nullptr; }
    virtual IFormatHandler* asFormatHandler() { return nullptr; }
    virtual IFeatureProvider* asFeatureProvider() { return nullptr; }
    virtual IUiContribution* asUiContribution() { return nullptr; }
};

#define MYCAD_PLUGIN_ENTRY(PluginClass)                                  \
    extern "C" MYCAD_PLUGIN_EXPORT IPlugin* createPlugin() {             \
        return new PluginClass();                                        \
    }                                                                    \
    extern "C" MYCAD_PLUGIN_EXPORT void destroyPlugin(IPlugin* p) {      \
        delete p;                                                        \
    }                                                                    \
    extern "C" MYCAD_PLUGIN_EXPORT uint32_t pluginAbiVersion() {         \
        return MYCAD_PLUGIN_ABI_VERSION;                                 \
    }
```

完整接口见 [§五 §5.4](./05-code-skeletons.md#54-iplugin--插件接口基类) + [ADR-0006](../adr/ADR-0006-plugin-double-abi.md)。

### 2.5.2 IPluginHost 接口

```cpp
class IPluginHost {
public:
    virtual ICommandBus& commandBus() = 0;

    virtual SubscriptionHandle subscribeEvent(EventTypeFilter, EventHandler) = 0;

    template <typename Service>
    Service* getService();
    template <typename Service>
    void registerService(std::unique_ptr<Service>);

    virtual IReadOnlyEntityRegistry& ecsRead() = 0;
    virtual void registerSystem(std::unique_ptr<ISystem>, SystemPhase) = 0;

    virtual IUiRegistry& ui() = 0;

    virtual ILogger& logger() = 0;
    virtual IProgressReporter& progress() = 0;

    virtual IGeometryPort& geometry() = 0;
};
```

**关键约束**：
- 插件**不能**直接获取 Aggregate 的可变引用 → 防止破坏不变式
- 插件**不能**直接修改 ECS Component → 只能通过发命令
- 插件**可以**注册自定义 System、自定义 Component 类型 → 扩展能力

### 2.5.3 生命周期状态机

```
              ┌─────────┐
              │  Found  │ ← 扫描到插件目录
              └────┬────┘
                   │ ABI 版本检查
                   ▼
              ┌─────────┐
              │ Loaded  │ ← dlopen / LoadLibrary 成功
              └────┬────┘
                   │ onLoad() 成功
                   ▼
              ┌─────────┐    onActivate() 失败
       ┌──────│Inactive │◄────┐
       │      └────┬────┘     │
       │           │          │
       │           ▼          │
       │      ┌─────────┐     │
       │      │ Active  │─────┘
       │      └────┬────┘
       │           │
       │           ▼
       │      ┌─────────┐
       └──────│Unloading│
              └────┬────┘
                   ▼
              ┌─────────┐
              │Unloaded │
              └─────────┘
```

### 2.5.4 ABI 兼容性策略

#### 双 ABI 边界

**Layer 1：核心 ABI（C 风格）** — 永远稳定
- 插件入口函数、版本查询、服务获取 → 全部 `extern "C"`
- 用整数版本号严格检查

**Layer 2：丰富 API（C++）** — 按主版本兼容
- 通过 `IPluginHost` 提供 C++ 接口
- 仅承诺**同一主版本号下 ABI 兼容**
- 主版本升级（1.x → 2.x）需要插件重新编译

详见 [ADR-0006](../adr/ADR-0006-plugin-double-abi.md)。

### 2.5.5 插件间通信：消息总线 vs 直接调用

**决策**：以**事件订阅为主**，**显式服务注册为辅**

| 场景 | 推荐方式 | 理由 |
|---|---|---|
| 插件 A 想知道"草图被修改了" | 订阅 `SketchUpdatedEvent` | 解耦 |
| 插件 A 调用插件 B 的"高级齿轮生成" | B 注册 `IGearGenerator` 服务 | 类型安全 |
| 插件 A 想拒绝某个命令（前置校验） | 订阅 `BeforeCommandEvent` 并设置 veto 标志 | 拦截器模式 |
| 插件 A 输出数据流给插件 B | 直接服务调用 + 共享数据结构 | 避免事件序列化开销 |

### 2.5.6 Python 插件的 pybind11 宿主

#### 整体架构

```
┌────────────────────────────────────┐
│   Python Plugin (foo.py)           │
│   class MyPlugin(mycad.Plugin):    │
│       def on_activate(self): ...   │
└──────────────┬─────────────────────┘
               │ pybind11 binding
               ▼
┌────────────────────────────────────┐
│   PythonPluginHost (C++)           │
│   - 启动 embedded CPython          │
│   - 加载 .py 文件                   │
│   - 桥接 IPlugin 调用               │
└──────────────┬─────────────────────┘
               │ implements IPlugin
               ▼
┌────────────────────────────────────┐
│   myCad Core (C++)                 │
└────────────────────────────────────┘
```

#### 关键设计决策

1. **进程内嵌入 vs 子进程**：嵌入。子进程方案 IPC 开销过大
2. **GIL 处理**：`py::gil_scoped_acquire` / `py::gil_scoped_release`
3. **错误传递**：Python 异常 → C++ `std::exception` → `std::expected<T, PluginError>`
4. **类型暴露**：Domain 层值对象显式 binding
5. **打包**：`.mycadpy` = zip 文件（py 源码 + manifest.json + 资源）

### 2.5.7 CAM / CAE / BIM 扩展点预留

#### 在 Phase 0/1 就要做的预留

```cpp
template <typename C>
void IEntityRegistry::registerComponentType();

enum class SystemPhase {
    EventConsume,
    GeometryUpdate,
    Constraint,
    UserDefined1,
    UserDefined2,
    Render,
};

class DomainEvent { /* 插件可继承定义自己的事件 */ };

class IFormatHandler {
public:
    virtual std::vector<FormatDescriptor> supportedFormats() const = 0;
    virtual std::expected<Document, FormatError> import(std::istream&) = 0;
    virtual std::expected<void, FormatError> export_(const Document&, std::ostream&) = 0;
};
```

**为什么 Phase 0 就要做**：扩展点必须从 Day 1 设计在架构里。事后改造会让插件接口断裂、所有第三方插件需重写。

---

## 2.6 几何内核集成设计

### 2.6.1 GeometryPort 接口抽象（六边形 Port）

```cpp
class IGeometryPort {
public:
    virtual std::expected<BRepHandle, GeomError>
        makeBox(double dx, double dy, double dz) = 0;
    virtual std::expected<BRepHandle, GeomError>
        prismaticExtrude(WireHandle wire, Vector3D direction, double depth) = 0;
    virtual std::expected<BRepHandle, GeomError>
        booleanUnion(BRepHandle a, BRepHandle b) = 0;
    virtual std::expected<BRepHandle, GeomError>
        fillet(BRepHandle shape, std::vector<EdgeRef> edges, double radius) = 0;
    virtual std::expected<TriangleMesh, GeomError>
        tessellate(BRepHandle shape, TessellationParams params) = 0;
    virtual std::vector<EdgeRef> edges(BRepHandle) = 0;
    virtual std::vector<FaceRef> faces(BRepHandle) = 0;
    virtual double volume(BRepHandle) = 0;
    virtual std::vector<std::byte> serialize(BRepHandle) = 0;
    virtual std::expected<BRepHandle, GeomError>
        deserialize(std::span<const std::byte>) = 0;
};
```

完整接口见 [§五 §5.1](./05-code-skeletons.md#51-igeometryport--几何内核端口接口)。

**为什么这样设计**：
- Domain 通过 `IGeometryPort` 调用几何操作，不知道 OCCT 存在
- 接口故意"业务化"，强制 Adapter 翻译
- 所有方法返回 `std::expected` → OCCT 异常被捕获并转换为业务错误码

### 2.6.2 OcctGeometryAdapter（具体实现）

```cpp
class OcctGeometryAdapter : public IGeometryPort {
    BRepHandleRegistry registry_;

public:
    std::expected<BRepHandle, GeomError>
    makeBox(double dx, double dy, double dz) override {
        try {
            TopoDS_Shape shape = BRepPrimAPI_MakeBox(dx, dy, dz).Shape();
            return registry_.store(shape);
        } catch (const Standard_Failure& e) {
            return std::unexpected(GeomError::fromOcct(e));
        }
    }
};
```

### 2.6.3 需封装的核心 OCCT API 列表

| 类别 | OCCT API | 封装为 IGeometryPort 方法 |
|---|---|---|
| **基础形体** | BRepPrimAPI_MakeBox/Cylinder/Cone/Sphere/Torus | makeBox/Cylinder/... |
| **拉伸/旋转** | BRepPrimAPI_MakePrism/MakeRevol | prismaticExtrude/revolve |
| **扫掠/放样** | BRepOffsetAPI_MakePipe/ThruSections | sweep/loft |
| **布尔** | BRepAlgoAPI_Fuse/Cut/Common/Section | boolean* |
| **倒角圆角** | BRepFilletAPI_MakeFillet/MakeChamfer | fillet/chamfer |
| **抽壳** | BRepOffsetAPI_MakeThickSolid | shell |
| **2D 草图** | BRepBuilderAPI_MakeWire/MakeEdge/MakeFace | makeWire* |
| **网格化** | BRepMesh_IncrementalMesh | tessellate |
| **拓扑遍历** | TopExp_Explorer | edges/faces/vertices |
| **测量** | GProp_GProps + BRepGProp | volume/area/centroid |
| **变换** | gp_Trsf + BRepBuilderAPI_Transform | applyTransform |
| **导入导出** | STEPControl_*, IGESControl_*, StlAPI | （在 IFormatHandler） |
| **序列化** | BinTools::Read/Write | serialize/deserialize |
| **AIS 交互** | AIS_InteractiveContext, AIS_Shape | （在 RenderPort） |

### 2.6.4 OCCT Handle 与 C++20 智能指针的共存

#### 问题

OCCT 使用自己的 `Handle<T>` 智能指针（基于内嵌引用计数），不是 `std::shared_ptr`。混用易出错。

#### 策略

**规则 1：OCCT 类型不出 Adapter 层**

```cpp
struct BRepHandle {
    uint64_t id;
    auto operator<=>(const BRepHandle&) const = default;
};

class BRepHandleRegistry {
    std::unordered_map<uint64_t, TopoDS_Shape> storage_;
    uint64_t nextId_ = 1;
public:
    BRepHandle store(TopoDS_Shape s) { /* ... */ }
    const TopoDS_Shape& get(BRepHandle h) const { return storage_.at(h.id); }
    void release(BRepHandle h) { storage_.erase(h.id); }
};
```

**规则 2：当必须暴露 OCCT 类型时，立刻包装**

```cpp
// 不好
void func(Handle(Geom_Curve) curve);

// 好
class CurveData {
    std::vector<Point3D> samples_;
    CurveKind kind_;
public:
    static CurveData fromOcct(const Handle(Geom_Curve)& c);
};
```

**规则 3：禁止在 Domain 层 #include 任何 OCCT 头**

通过 CMake + CI 强制：

```cmake
target_include_directories(mycad_domain PRIVATE
    ${CMAKE_SOURCE_DIR}/src/domain/include
)
# 故意不 link OCCT，编译期保证零依赖
```

### 2.6.5 OCCT 对象与 ECS Entity 的生命周期同步

#### 关键问题

```
ECS Entity 销毁时 → 关联的 BRepHandle 必须释放
否则 OCCT TopoDS_Shape 永久占用内存
```

#### 解决方案：Component 的 RAII

```cpp
struct BRepGeometryComponent {
    BRepHandle handle;
    BoundingBox bbox;

    ~BRepGeometryComponent() {
        if (handle.id != 0)
            geometryAdapter().release(handle);
    }

    BRepGeometryComponent(const BRepGeometryComponent&) = delete;
    BRepGeometryComponent& operator=(const BRepGeometryComponent&) = delete;
    BRepGeometryComponent(BRepGeometryComponent&&) noexcept = default;
    BRepGeometryComponent& operator=(BRepGeometryComponent&&) noexcept = default;
};
```

EnTT 的 `registry.destroy(entity)` 会触发所有 Component 析构 → 自动释放 OCCT 对象。

**为什么不用 shared_ptr**：
- 几何对象多份共享会让"修改一个影响所有"的 bug 难以追查
- 独占所有权 + 显式 clone 更安全
- 真正需要共享的几何用单独的 `BRepCacheComponent` + 引用计数

---

## 2.7 渲染架构设计

### 2.7.1 OpenGL 4.5+ vs Vulkan

**决策**：**OpenGL 4.5 + DSA**（详见 [ADR-0005](../adr/ADR-0005-opengl-not-vulkan.md)）

| 维度 | OpenGL 4.5 | Vulkan |
|---|---|---|
| **CAD 行业现状** | SolidWorks, Inventor, Rhino, FreeCAD 主流 | 几乎无 CAD 软件采用 |
| **多线程渲染** | 受限 | 原生支持 |
| **驱动稳定性** | 极成熟 | 各厂商驱动质量参差 |
| **代码量** | 中等 | 是 GL 的 3-5 倍 |
| **学习曲线** | 中 | 极陡 |
| **个人开发者可控性** | 高 | 低 |
| **CAD 渲染瓶颈** | 几何复杂度而非渲染调用 | 同 |
| **未来路径** | 可平滑过渡 wgpu / Vulkan 后端 | 反向迁移困难 |

**为什么 OpenGL 而非 Vulkan**：CAD 渲染瓶颈在"几何 LOD + 拓扑剔除 + 大装配体管理"，不在 draw call 数量。Vulkan 的优势点对 CAD 不重要。

**未来过渡策略**：通过 `IRenderPort` 抽象封装，远期可加 wgpu Backend（覆盖 Web/WASM）。详见 [§九 §9.2.5](./09-self-host-strategy.md)。

### 2.7.2 渲染管线总览

```
Pass 1: Geometry / Solid     — PBR、实例化、MultiDrawIndirect
Pass 2: Edge / Wireframe     — 显式边渲染、隐藏线消除
Pass 3: Selection / Highlight — 描边（Stencil）、高亮叠加
Pass 4: Transparent / Section Plane — OIT、剖切面 clip-plane
Pass 5: UI Overlay           — 草图实体、约束图标、SDF 文字
```

### 2.7.3 选择集 / 高亮 / 透明 / 剖切

#### 选择（Picking）

| 方案 | 精度 | 性能 | 推荐场景 |
|---|---|---|---|
| **CPU Ray-BRep** | 极高（亚像素） | 慢（百毫秒） | 精确点击边 / 顶点 |
| **GPU Color Picking** | 像素级 | 快 | 普通点击 |
| **Hybrid** | 高 | 快 | 默认采用 |

**Hybrid 策略**：
1. 鼠标按下时，GPU Color Picking 获取候选 Entity
2. 在该 Entity 的 BRep 上做精确 CPU Ray Trace 找到最近的 Edge / Face / Vertex
3. 返回 `SelectionHit{entityId, topologyKind, topologyId}`

#### 高亮（Highlight / Selected）

- Stencil Buffer + 后处理描边（Sobel 边缘检测）→ 0.5ms 内完成
- 选中颜色叠加：着色器读 `SelectionState` SSBO

#### 透明

- OIT（Order-Independent Transparency）用 Weighted Blended OIT（McGuire）
- 比传统 Alpha-Sort 快且无需排序

#### 剖切面

- `gl_ClipDistance[N]` 在顶点着色器输出
- 切面边界绘制：CPU 计算 BRep × Plane 截面 → 上传 line strip
- 可同时启用最多 6 个剖切面

### 2.7.4 大型装配体 LOD 与实例化

#### LOD 策略

```cpp
struct MeshLodChain {
    TriangleMesh full;        // 0 - 完整网格
    TriangleMesh medium;      // 1 - X/4
    TriangleMesh low;         // 2 - X/16
    TriangleMesh impostor;    // 3 - 公告板
};

class LODSystem {
    void selectLod(EntityId eid, double distanceToCamera, double pixelSize) {
        if (pixelSize < 4) ecs_.set(eid, MeshLodLevel::Impostor);
        else if (pixelSize < 32) ecs_.set(eid, MeshLodLevel::Low);
        else if (pixelSize < 128) ecs_.set(eid, MeshLodLevel::Medium);
        else ecs_.set(eid, MeshLodLevel::Full);
    }
};
```

#### 实例化（Instancing）

```cpp
struct InstancedMeshComponent {
    MeshId baseMesh;
    std::vector<Eigen::Matrix4f> instanceTransforms;
};

void render() {
    auto view = ecs.view<InstancedMeshComponent, MaterialComponent>();
    for (auto&& [eid, mesh, mat] : view.each()) {
        gpu.bindMesh(mesh.baseMesh);
        gpu.bindMaterial(mat);
        gpu.uploadInstanceData(mesh.instanceTransforms);
        gpu.drawInstanced(mesh.instanceTransforms.size());
    }
}
```

### 2.7.5 AIS（OCCT Visualization）vs 自建渲染层

**决策**：**自建渲染层**

| 维度 | AIS | 自建 |
|---|---|---|
| **集成速度** | 1 周可演示 | 1 个月起步 |
| **架构契合** | 与 ECS 严重冲突 | 自然融入 ECS |
| **现代渲染特性** | OpenGL 立即模式遗留 | 自由 |
| **性能可控** | 黑盒 | 完全可控 |
| **调试** | 难 | 易 |
| **未来 Web/Vulkan 后端** | 不可能 | 可行 |
| **工程量** | 极小 | 大 |

**为什么自建**：
- AIS 与 ECS 是"两套对象管理系统"，强行融合会引入大量同步代码
- AIS 的 OpenGL 后端使用方式陈旧（GL2 风格）
- CAD 的核心差异化恰好在渲染体验，不能依赖一个老旧黑盒

**Phase 0 例外**：可以先用 AIS 出最快 demo，但 Phase 0 末必须替换为自建渲染层。

---

## 2.8 2D 草图约束求解器设计

### 2.8.1 求解器选型

| 方案 | 复杂度 | 求解能力 | 许可 | 推荐 |
|---|---|---|---|---|
| **自实现 Eigen LM** | 高 | 中 | LGPL 兼容 | ❌（备选） |
| **集成 SolveSpace** | 中 | 高 | GPL v3 ⚠️ | ⚠️ 协议冲突 |
| **集成 Sketcher Solver from FreeCAD** | 中 | 高 | LGPL | ✅ |
| **PlaneGCS（FreeCAD 用的求解器）** | 低 | 高 | LGPL | ✅✅ 推荐 |
| **OCCT 自带 2D 求解器** | 低 | 弱 | LGPL | ❌ |

**推荐：集成 PlaneGCS**

**为什么**：
- LGPL 许可，与 myCad 协议完全兼容
- 已被 FreeCAD 在生产中验证 10+ 年
- 算法基础是 BFGS + DogLeg，对欠/过约束有合理处理
- C++ 接口干净，不依赖 Qt

**Plan B**：自实现 Eigen LM — 工程量约 1-2 个月（详见 [§九 §9.2.4](./09-self-host-strategy.md)）

### 2.8.2 约束类型系统

#### 几何约束

| 名称 | 涉及实体 | 数学描述 |
|---|---|---|
| Coincident | 2 个点 | p1 = p2 |
| Horizontal | 1 条直线 | y1 = y2 |
| Vertical | 1 条直线 | x1 = x2 |
| Parallel | 2 条直线 | dir1 × dir2 = 0 |
| Perpendicular | 2 条直线 | dir1 · dir2 = 0 |
| Tangent | 直线 + 圆弧 / 圆弧 + 圆弧 | 距离方程 |
| Concentric | 2 个圆/弧 | 圆心重合 |
| Equal | 2 个长度/半径 | 数值相等 |
| Symmetric | 2 个点 + 1 条对称轴 | 距离 + 方向 |
| OnLine | 1 个点 + 1 条直线 | 点到直线距离 = 0 |

#### 尺寸约束

| 名称 | 涉及实体 | 参数 |
|---|---|---|
| DistancePP | 2 个点 | distance |
| DistancePL | 1 点 + 1 直线 | distance |
| Angle | 2 条直线 | angle (rad) |
| Radius | 1 个圆/弧 | radius |
| Diameter | 1 个圆 | diameter |

#### 类型系统的 C++ 表达

```cpp
struct Coincident { SketchPointId p1, p2; };
struct Horizontal { SketchLineId line; };
struct Parallel { SketchLineId l1, l2; };
struct DistancePP { SketchPointId p1, p2; double distance; };

using ConstraintSpec = std::variant<
    Coincident, Horizontal, Vertical, Parallel, Perpendicular,
    Tangent, Concentric, Equal, Symmetric, OnLine,
    DistancePP, DistancePL, Angle, Radius, Diameter
>;

auto residual = std::visit(overloaded{
    [](const Coincident& c) { return computeCoincidentResidual(c); },
    [](const Horizontal& h) { return computeHorizontalResidual(h); },
}, spec);
```

### 2.8.3 欠约束 / 过约束的检测与用户反馈

#### 自由度（DOF）分析

```cpp
struct DofAnalysis {
    int totalVariables;
    int totalConstraintEqs;
    int dof;
    std::vector<RedundantConstraint> redundants;
    std::vector<ConflictingConstraint> conflicts;
};

DofAnalysis analyze(const SketchConstraintSystem& sys);
```

- **dof > 0**：欠约束 → UI 标灰显示
- **dof = 0**：完全约束（理想） → UI 标黑/绿显示
- **dof < 0** 或 **存在冲突方程**：过约束 → 高亮冲突约束并提示"删除哪一个可以解决"

#### 用户反馈 UX

```
[底部状态栏] 草图状态: ⚠ 欠约束（剩余 3 个自由度）  [详情]
[底部状态栏] 草图状态: ❌ 过约束 - 检测到 1 个冲突  [详情]
```

### 2.8.4 求解器 C++ API

```cpp
class IConstraintSolver {
public:
    struct SolveRequest {
        std::vector<Variable> variables;
        std::vector<ConstraintEquation> equations;
        SolverConfig config;
        std::optional<std::vector<double>> initialGuess;
    };

    struct SolveResult {
        SolveStatus status;
        std::vector<double> solution;
        double finalResidual;
        size_t iterations;
        std::chrono::microseconds elapsed;
    };

    virtual std::expected<SolveResult, SolverError>
        solve(const SolveRequest& req) = 0;

    virtual DofAnalysis analyze(const SolveRequest& req) = 0;

    virtual std::expected<SolveResult, SolverError>
        incrementalSolve(SolverHandle handle,
                          std::span<const VariableUpdate> changes) = 0;
};

class PlaneGcsSolver : public IConstraintSolver { /* ... */ };
class EigenLMSolver : public IConstraintSolver { /* ... */ };
```

**为什么有 incrementalSolve**：拖拽时每帧重做完整 LM 分解会卡。增量求解复用雅可比 LU 分解 → 60 fps 流畅交互。

---

## 2.9 文件格式与互操作设计

### 2.9.1 原生格式 .mycad

#### 物理结构

```
foo.mycad （ZIP 容器）
├── manifest.json          # 文件元数据
├── events.db              # SQLite 数据库：完整事件流
├── snapshots/             # 二进制快照
│   └── feature_tree_<id>_v100.snap
├── geometry_cache/        # OCCT BRep 二进制缓存
│   └── feature_<id>.brep
├── plugins.json           # 该文档使用的插件列表
└── thumbnails/256x256.png
```

**为什么用 ZIP 容器**：
- 单文件好分发
- 内部多文件好做增量保存（Git 友好）
- 可用任何 ZIP 工具检查内容
- ZIP 已有压缩

#### Logical Schema

```json
{
    "schemaVersion": "1.0.0",
    "applicationVersion": "myCad 0.5.0",
    "created": "2026-05-03T12:34:56Z",
    "createdBy": "user@example.com",
    "rootAggregateId": "fa5ce7c4-...",
    "documentKind": "Part" | "Assembly" | "Drawing"
}
```

### 2.9.2 序列化格式：FlatBuffers

| 格式 | 序列化速度 | 反序列化速度 | 二进制大小 | Schema 演进 | 推荐 |
|---|---|---|---|---|---|
| **JSON** | 慢 | 慢 | 大 | 极好 | ❌ 仅 manifest |
| **Protocol Buffers** | 中 | 中 | 中 | 好 | ⚠️ |
| **FlatBuffers** | 慢 | **零拷贝** | 小 | 好 | ✅ |
| **MessagePack** | 中 | 中 | 小 | 弱 | ❌ |
| **Cap'n Proto** | 快 | 零拷贝 | 小 | 好 | ⚠️ 较冷门 |

**决策：FlatBuffers**

**为什么**：
- 反序列化零拷贝 → 打开大文档关键
- 二进制紧凑 → 千万级事件场景下文件大小可控
- Schema 演进良好 → 老版本 myCad 能读新版本文件

#### Schema 示例

```fbs
namespace mycad.events;

table Point2D {
    x: double;
    y: double;
}

enum SketchEntityKind: byte {
    Line = 0,
    Circle = 1,
    Arc = 2,
}

table SketchEntityAdded {
    sketch_id: string;
    entity_id: string;
    kind: SketchEntityKind;
    points: [Point2D];
}

union DomainEventPayload {
    SketchEntityAdded,
}

table DomainEventEnvelope {
    event_id: string;
    aggregate_id: string;
    aggregate_type: string;
    version: uint64;
    operation_id: string;
    timestamp_ns: uint64;
    user_id: string;
    payload: DomainEventPayload;
}

root_type DomainEventEnvelope;
```

### 2.9.3 STEP / IGES / DXF 集成

#### 通过 OCCT 但单独抽象为 IFormatHandler

```cpp
class IFormatHandler {
public:
    virtual std::vector<FormatDescriptor> supported() const = 0;
    virtual bool canImport(std::string_view extension) const = 0;
    virtual bool canExport(std::string_view extension) const = 0;

    virtual std::expected<ImportResult, FormatError>
        import_(std::istream& in, ImportOptions opts) = 0;

    virtual std::expected<void, FormatError>
        export_(std::ostream& out, const ExportSubject& subject, ExportOptions opts) = 0;
};

class StepHandler : public IFormatHandler { /* OCCT STEPControl_Reader/Writer */ };
class IgesHandler : public IFormatHandler { /* OCCT IGESControl_* */ };
class DxfHandler : public IFormatHandler { /* libdxfrw or OCCT DXF Reader */ };
class StlHandler : public IFormatHandler { /* OCCT StlAPI */ };
class ObjHandler : public IFormatHandler { /* 自实现，简单 */ };
```

#### 导入策略

```cpp
struct ImportResult {
    std::vector<DomainEvent> generatedEvents;
    ImportDiagnostics diagnostics;
    BRepHandle rootShape;
};
```

**关键设计**：导入不是"塞进内存"，而是**生成等价的领域事件**。这保证了 Undo/Redo、序列化、协同对导入对象同样有效。

#### 导出策略

```cpp
struct ExportSubject {
    std::variant<
        BRepHandle,
        FeatureTreeId,
        AssemblyId,
        std::vector<EntityId>
    > target;
};
```

### 2.9.4 格式转换端口适配器

```
┌─────────────────────────────────────┐
│  Application Layer                  │
│   ImportCommand/ExportCommand       │
└─────────────┬───────────────────────┘
              ▼
┌─────────────────────────────────────┐
│  IFormatHandlerRegistry             │ ← 服务定位
└─────────────┬───────────────────────┘
              │ 按扩展名分发
        ┌─────┴─────┬─────────┬──────────┐
        ▼           ▼         ▼          ▼
   ┌────────┐ ┌────────┐ ┌────────┐ ┌────────────┐
   │  STEP  │ │  IGES  │ │  DXF   │ │ Plugin     │
   │Handler │ │Handler │ │Handler │ │Handlers    │
   └────┬───┘ └────┬───┘ └────────┘ └────────────┘
        │ 调用
        ▼
   ┌─────────────────────┐
   │  IGeometryPort      │
   └─────────────────────┘
```

**为什么 FormatHandler 与 GeometryPort 分开**：
- GeometryPort 关心"几何运算"
- FormatHandler 关心"流式 IO + 格式语义"
- 职责分离让插件可以扩展新格式

### 2.9.5 版本演进策略

#### Schema 版本号

manifest.json 中的 `schemaVersion` 用 SemVer：
- **PATCH**：完全兼容
- **MINOR**：向下兼容（新加可选字段）
- **MAJOR**：破坏性变更（必须迁移）

#### 迁移策略

```cpp
class IDocumentMigrator {
public:
    virtual SchemaVersion fromVersion() const = 0;
    virtual SchemaVersion toVersion() const = 0;
    virtual std::expected<void, MigrationError>
        migrate(DocumentBundle& bundle) = 0;
};

class Migrator_1_0_to_1_1 : public IDocumentMigrator {
    // 给所有历史事件填充 user_id = "legacy"
};

class MigrationPipeline {
    std::expected<void, MigrationError>
        upgradeToLatest(DocumentBundle& bundle);
};
```

**为什么从一开始就要做迁移框架**：哪怕 v0.1 只有一个 schema，也要有 schemaVersion 字段 + Migrator 接口。等到 v1.0 才补就晚了 — 历史文件没有 schemaVersion 字段，没法判断怎么升级。

---

> **最后修订**：2026-05（首次拆分自 ARCHITECTURE.md）
