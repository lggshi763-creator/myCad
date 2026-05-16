# Sprint 0.5 — UI 完善 + 文件 IO（Week 9-10，2026-06-02 ~ 06-13）

> **工具分工**：🟦 Claude Code（全程直接实现）·🟩 人工（决策/验证）
>
> **每日编码时间上限**：2-3 小时。Day 1-10 覆盖 10 个工作日。
>
> **参考**：[ADR-0007](../adr/ADR-0007-entt-as-ecs.md) · [06-roadmap.md §Sprint 0.5](../architecture/06-roadmap.md) · [inbox.md §2026-05-16](../inbox.md)

---

## 阶段目标（一句话）

**完整的"创建 → 保存 → 关闭 → 重开 → 立方体还在"闭环**：主窗口加入 File 菜单，事件持久化到 SQLite，场景打包为 `.mycad` ZIP 容器，同时修复退出时 GL 崩溃并改善光照。

---

## 前置确认（Sprint 启动时已满足）

- [x] Sprint 0.4 全量通过：`mycad_app` 可启动，立方体可见，Camera 可 orbit
- [x] Qt6、OCCT、EnTT 均通过 vcpkg 安装
- [x] `InMemoryEventStore` 接口稳定（`IEventStore` 已定义）
- [ ] vcpkg 新增 `sqlite3`（或 `sqlitecpp`）、`libzip` 依赖（T0 确认）

---

## Sprint 验收清单

- [ ] **V1** File → Save As：当前场景写入 `*.mycad` 文件（ZIP 容器，含 `manifest.json` + `events.db`）
- [ ] **V2** 关闭程序，File → Open，重新加载 `.mycad`：立方体重现，视角默认
- [ ] **V3** 关闭主窗口**无崩溃 / 无异常**（GL context cleanup 修复）
- [ ] **V4** `SqliteEventStore` 单元测试全部 PASS（覆盖 append / latestVersion / load / replay）
- [ ] **V5** 立方体有正确漫反射光照（per-vertex normals 计算，不再全灰）
- [ ] **V6** `src/domain/` 零外部依赖仍成立（CI domain-isolation job 持续绿）

---

## 任务总览

| # | 任务 | 估时 | 验收 |
|---|---|---|---|
| T0 | vcpkg 新增 `sqlite3` + `libzip` | 0.5d | 编译通过 |
| T1 | MainWindow 菜单栏（File: New / Save As… / Open… / Exit） | 1d | 菜单项可点击，Slot 占位 |
| T2 | `SqliteEventStore`（IEventStore 的 SQLite 实现） | 3d | V4 单元测试 |
| T3 | `.mycad` 文件格式（ZIP 容器读写） | 1.5d | 能打包 / 解包 events.db + manifest.json |
| T4 | File → Save As 逻辑 | 1d | V1 |
| T5 | File → Open 逻辑 + 端到端验证 | 1d | V2 |
| T6 | GL context cleanup（关闭不崩） | 0.5d | V3 |
| T7 | Per-vertex normals（改善光照） | 0.5d | V5 |
| T8 | Sprint 收尾（devlog / tag / playbook 勾选） | 0.5d | V1-V6 全勾 |

**估时合计**：~9.5d × ~2.5h = **~24h**（含 buffer）

---

## 架构决策（Sprint 内有效）

| 决策 | 理由 |
|---|---|
| SQLite 直接用 C API，RAII 封装（不用 ORM） | 事件模型极简（append-only），ORM 是过度设计 |
| `.mycad` = ZIP（libzip），内含 `events.db` + `manifest.json` | ZIP 可扩展（以后加 thumbnails、assets），manifest 提供版本信息 |
| `SqliteEventStore` 放 `infrastructure/eventsourcing/` | 与 `InMemoryEventStore` 同层，保持 IEventStore 接口稳定 |
| File Open 时：清空 ECS + 重放事件重建场景 | 符合事件溯源原则，避免引入快照（快照留 Sprint 1.C2） |
| GL cleanup 通过 `QOpenGLContext::aboutToBeDestroyed` 信号触发 | Qt 官方推荐做法；destructor 里调 GL 函数是 UB |
| Per-vertex normals 用 OCCT `BRep_Tool::Normal` 或手动面法线平均 | 利用已有 OCCT 依赖，无需新引入几何库 |

---

## 目录结构（Sprint 0.5 新增 / 修改）

```
src/
├── infrastructure/
│   ├── eventsourcing/
│   │   ├── include/mycad/infrastructure/
│   │   │   ├── InMemoryEventStore.hpp      ← 已有
│   │   │   └── SqliteEventStore.hpp        ← T2 新增
│   │   ├── InMemoryEventStore.cpp          ← 已有
│   │   └── SqliteEventStore.cpp            ← T2 新增
│   ├── persistence/                        ← T3 新增目录
│   │   ├── include/mycad/infrastructure/
│   │   │   └── MycadFileStore.hpp          ← T3（ZIP 读写接口）
│   │   └── MycadFileStore.cpp              ← T3
│   └── rendering/
│       └── OpenGLRenderAdapter.cpp         ← T6 T7 修改（cleanup + normals）
├── ui/
│   ├── include/mycad/ui/
│   │   ├── MainWindow.hpp                  ← T1 修改（加菜单/槽）
│   │   └── ViewportWidget.hpp              ← T6 修改（aboutToBeDestroyed）
│   ├── MainWindow.cpp                      ← T1 T4 T5 修改
│   └── ViewportWidget.cpp                  ← T6 修改

tests/
└── infrastructure/
    └── sqlite_eventstore_test.cpp          ← T2 新增
```

---

## 每日任务分解

---

### Day 0（~1h）：T0 — vcpkg 新增依赖

在 `vcpkg.json` 加入：

```json
"sqlite3",
"libzip"
```

在各层 `CMakeLists.txt` 里 `find_package` 并链接：
- `SqliteEventStore.cpp` PRIVATE → `unofficial::sqlite3::sqlite3`
- `MycadFileStore.cpp` PRIVATE → `libzip::zip`

**验收**：`cmake --preset local-debug` 无报错，两个包编译链接通过。

---

### Day 1（~2.5h）：T1 — MainWindow 菜单栏

> **前置**：T0 完成

在 `MainWindow.hpp` 中增加：

```cpp
private slots:
    void onFileNew();
    void onFileSaveAs();
    void onFileOpen();

private:
    void setupMenus();
    QString currentFilePath_;          // 当前打开的 .mycad 路径（空 = 未保存）
```

在 `MainWindow.cpp` 中：

```cpp
void MainWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"),         this, &MainWindow::onFileNew,     QKeySequence::New);
    fileMenu->addAction(tr("&Save As..."),  this, &MainWindow::onFileSaveAs,  QKeySequence::SaveAs);
    fileMenu->addAction(tr("&Open..."),     this, &MainWindow::onFileOpen,    QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"),        qApp, &QApplication::quit,        QKeySequence::Quit);
}

// onFileNew / onFileSaveAs / onFileOpen 先为空槽（T4/T5 填充）
```

在 `MainWindow` 构造函数末尾调用 `setupMenus()`。

**验收**：
- [ ] 菜单栏出现，四项均可点击
- [ ] 快捷键 Ctrl+S（Save As）、Ctrl+O（Open）响应（槽为空，不 crash）

---

### Day 2-4（~2.5h × 3）：T2 — SqliteEventStore

> **前置**：T0 完成；`IEventStore` 接口不变

#### SqliteEventStore.hpp

```cpp
/// @brief IEventStore 的 SQLite 持久化实现。
///
/// 数据库 schema（单表）：
///   CREATE TABLE IF NOT EXISTS events (
///       aggregate_id  BLOB NOT NULL,   -- 16 bytes AggregateId
///       version       INTEGER NOT NULL,
///       type_name     TEXT NOT NULL,
///       payload_json  TEXT NOT NULL,
///       recorded_at   INTEGER NOT NULL  -- Unix epoch ms
///   );
class SqliteEventStore final : public domain::IEventStore {
public:
    /// @brief 打开或创建 SQLite 数据库文件。
    /// @throws std::runtime_error 若文件无法打开。
    explicit SqliteEventStore(std::string_view dbPath);
    ~SqliteEventStore() override;

    // IEventStore overrides
    void        append(const domain::AggregateId& id,
                       domain::Version             expectedVersion,
                       std::span<const std::unique_ptr<domain::DomainEvent>> events) override;
    domain::Version latestVersion(const domain::AggregateId& id) const override;
    std::vector<std::unique_ptr<domain::DomainEvent>>
                load(const domain::AggregateId& id) const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

#### Impl 结构

```cpp
struct SqliteEventStore::Impl {
    sqlite3* db{nullptr};
    sqlite3_stmt* stmtInsert{nullptr};
    sqlite3_stmt* stmtMaxVer{nullptr};
    sqlite3_stmt* stmtLoad{nullptr};
};
```

#### 序列化策略（Sprint 0.5 简化版）

每个 `DomainEvent` 序列化为 JSON 字符串（手写，不引入 nlohmann/json）：
- `BoxCreatedEvent` → `{"dx":2.0,"dy":3.0,"dz":4.0,"handle":42}`

反序列化用 `EventTypeRegistry`（已有）+ 简单字符串解析。
> **注**：Sprint 1.C2 再换 FlatBuffers；Sprint 0.5 先让流程跑通。

#### 单元测试（`tests/infrastructure/sqlite_eventstore_test.cpp`）

```
TEST_CASE "SqliteEventStore - append and latestVersion"
TEST_CASE "SqliteEventStore - load replays events in order"
TEST_CASE "SqliteEventStore - optimistic concurrency: wrong expectedVersion throws"
TEST_CASE "SqliteEventStore - two aggregates are isolated"
TEST_CASE "SqliteEventStore - reopen db retains events"   ← 关键：关闭再打开文件
```

**验收（V4）**：
- [ ] 所有 SqliteEventStore 测试 PASS
- [ ] `tests/CMakeLists.txt` 加入 `mycad_infra_tests`（或扩展已有 target）

---

### Day 5（~2.5h）：T3 — .mycad 文件格式

> **前置**：T2 完成

#### 文件格式规范

```
<file>.mycad  (ZIP 容器)
├── manifest.json          ← 格式版本、创建时间、缩略图路径
└── events.db              ← SqliteEventStore 的数据库文件
```

`manifest.json` 最小结构：
```json
{
  "formatVersion": 1,
  "appVersion": "0.0.1",
  "createdAt": "2026-06-05T10:00:00Z",
  "thumbnail": null
}
```

#### MycadFileStore.hpp

```cpp
/// @brief .mycad ZIP 容器的读写工具类（非 IEventStore 实现）。
class MycadFileStore {
public:
    /// @brief 将 events.db 文件打包为 .mycad ZIP 文件。
    /// @throws std::runtime_error on failure.
    static void save(std::string_view dbPath, std::string_view outputPath);

    /// @brief 从 .mycad ZIP 文件解包 events.db 到临时目录，返回 db 路径。
    /// @throws std::runtime_error on failure.
    [[nodiscard]] static std::string load(std::string_view mycadPath,
                                          std::string_view extractDir);
};
```

实现用 `libzip`：`zip_open` / `zip_source_file` / `zip_file_add` / `zip_close`。

**验收**：
- [ ] 能把一个 `test.db` 打包成 `test.mycad`，解包后内容一致

---

### Day 6（~2.5h）：T4 — File → Save As

> **前置**：T1 T2 T3 完成

`MainWindow` 需要持有 `SqliteEventStore`（而不是 `InMemoryEventStore`）。
调整 `main.cpp` composition root：默认使用 `SqliteEventStore`（路径为临时文件），有文件名后切换到正式路径。

```cpp
// main.cpp 调整：
auto dbPath = (QStandardPaths::writableLocation(QStandardPaths::TempLocation)
               + "/mycad_session.db").toStdString();
auto store  = std::make_shared<infrastructure::SqliteEventStore>(dbPath);
```

`MainWindow::onFileSaveAs()`：

```cpp
void MainWindow::onFileSaveAs() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save As"), QString(), tr("myCad Files (*.mycad)"));
    if (path.isEmpty()) return;

    // 将当前 session db 打包为 .mycad
    infrastructure::MycadFileStore::save(currentDbPath_, path.toStdString());
    currentFilePath_ = path;
    setWindowTitle(QFileInfo(path).fileName() + " — myCad");
}
```

**验收（V1）**：
- [ ] Save As 对话框弹出，选路径后生成 `.mycad` 文件
- [ ] 用 7-zip 打开确认 `manifest.json` + `events.db` 均存在

---

### Day 7（~2.5h）：T5 — File → Open + 端到端验证

> **前置**：T4 完成

`MainWindow::onFileOpen()`：

```cpp
void MainWindow::onFileOpen() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open"), QString(), tr("myCad Files (*.mycad)"));
    if (path.isEmpty()) return;

    // 1. 解包到 temp dir
    const std::string dbPath = infrastructure::MycadFileStore::load(
        path.toStdString(), tempDir_.path().toStdString());

    // 2. 重建 SqliteEventStore
    auto newStore = std::make_shared<infrastructure::SqliteEventStore>(dbPath);

    // 3. 清空 ECS，重放所有事件重建实体
    ecs_->clear();
    replayAll(*newStore);

    // 4. 刷新视口
    viewport_->update();
    currentFilePath_ = path;
    setWindowTitle(QFileInfo(path).fileName() + " — myCad");
}
```

`replayAll()` 遍历 store 内所有 aggregate，对每个已知 ID replay `BoxCreatedEvent` → 重新 uploadMesh。

> Sprint 0.5 简化版：aggregate ID 列表从 ECS 快照中取（尚无），先写死从 store 查第一个 aggregate。
> **Sprint 1.C2** 引入真正的 replayAll（全量 aggregate 枚举）。

**端到端验证（V2）**：
- [ ] 创建立方体 → Save As `box.mycad` → 关闭程序 → 重启 → Open `box.mycad` → 立方体重现

---

### Day 8（~2h）：T6 — GL context cleanup

> **前置**：无（独立修复，任何时候可做）
>
> 对应 inbox 条目：2026-05-16 §渲染/UI

#### 修改 `OpenGLRenderAdapter`

增加 `cleanup()` 方法，把所有 `glDelete*` 调用从 destructor 移入其中：

```cpp
// .hpp
void cleanup() noexcept;   // 必须在 GL context 有效时调用

// .cpp
void OpenGLRenderAdapter::cleanup() noexcept {
    if (!impl_->initialized || !impl_->gl) return;
    auto& gl = *impl_->gl;
    for (auto& [id, entry] : impl_->meshes) freeMeshEntry(gl, entry);
    impl_->meshes.clear();
    freeMeshEntry(gl, impl_->testTriangle);
    impl_->hasTestTriangle = false;
    if (impl_->shaderProgram != 0u) {
        gl.glDeleteProgram(impl_->shaderProgram);
        impl_->shaderProgram = 0;
    }
    impl_->initialized = false;
}

// destructor 只 reset PIMPL，不调 GL：
OpenGLRenderAdapter::~OpenGLRenderAdapter() = default;
```

#### 修改 `ViewportWidget::initializeGL()`

```cpp
void ViewportWidget::initializeGL() {
    if (adapter_) {
        adapter_->initialize();

        // 确保 GL 资源在 context 销毁前清理，而不是在 destructor 里
        connect(context(), &QOpenGLContext::aboutToBeDestroyed,
                this, [this]() {
                    makeCurrent();
                    if (adapter_) adapter_->cleanup();
                    doneCurrent();
                }, Qt::DirectConnection);
    }
    emit glReady();
}
```

**验收（V3）**：
- [ ] 关闭主窗口：无异常、无崩溃、无 Debug 输出中的 GL 错误

---

### Day 9（~1.5h）：T7 — Per-vertex normals

> **前置**：无（独立；对应 Sprint 0.4 中 `// TODO(@dev, sprint-0.5): compute per-vertex normals`）

在 `OpenGLRenderAdapter::uploadMesh()` 中，替换全零法线占位符为真实法线：

`TriangleMesh` 已有 `vertices`（每 3 个 float = 一个顶点）和 `indices`（三角面）。
用面法线平均（flat normal 展平到顶点）：

```cpp
// 计算 per-face 法线，写入对应顶点（每个顶点最多被多个面共享，取平均）
std::vector<float> normals(mesh.vertices.size(), 0.f);
const std::size_t triCount = mesh.indices.size() / 3;
for (std::size_t i = 0; i < triCount; ++i) {
    const uint32_t i0 = mesh.indices[i * 3 + 0];
    const uint32_t i1 = mesh.indices[i * 3 + 1];
    const uint32_t i2 = mesh.indices[i * 3 + 2];
    // 取三个顶点坐标
    const float* v0 = &mesh.vertices[i0 * 3];
    const float* v1 = &mesh.vertices[i1 * 3];
    const float* v2 = &mesh.vertices[i2 * 3];
    // 叉积求面法线
    float e1[3] = {v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2]};
    float e2[3] = {v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2]};
    float n[3]  = {e1[1]*e2[2]-e1[2]*e2[1],
                   e1[2]*e2[0]-e1[0]*e2[2],
                   e1[0]*e2[1]-e1[1]*e2[0]};
    // 累加到三个顶点
    for (int j = 0; j < 3; ++j) {
        normals[mesh.indices[i*3+0]*3+j] += n[j];
        normals[mesh.indices[i*3+1]*3+j] += n[j];
        normals[mesh.indices[i*3+2]*3+j] += n[j];
    }
}
// 归一化
const std::size_t vertexCount = mesh.vertices.size() / 3;
for (std::size_t i = 0; i < vertexCount; ++i) {
    float* n = &normals[i * 3];
    const float len = std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    if (len > 1e-6f) { n[0] /= len; n[1] /= len; n[2] /= len; }
}
```

**验收（V5）**：
- [ ] 立方体六个面有明显的明暗变化（不再全灰/全黑）
- [ ] 删除 Sprint 0.4 的 `// TODO(@dev, sprint-0.5)` 注释

---

### Day 10（~1.5h）：T8 — Sprint 收尾

1. **全量 build + test**：
   ```powershell
   cmake --build E:/myCad/build/local-debug --config Debug
   ctest --test-dir E:/myCad/build/local-debug -C Debug --output-on-failure
   ```

2. **端到端剧本手动验证**（V1-V6 全部走一遍）

3. **devlog**：写 `docs/devlog/2026-W24.md`（或对应周）

4. **tag**：`git tag sprint-0.5-done`（不 push，用户决定）

5. 勾选本文件所有验收项

---

## 给下个 Sprint 的预热（Sprint 0.6）

- **主题**：插件骨架 + Phase 0 收尾（IPlugin / PluginRegistry / AddressSanitizer 全量）
- **Sprint 0.6 启动前需要确认**：
  1. `.mycad` 格式的 format version 定义稳定（后续插件数据可扩展字段）
  2. `IEventStore` 接口最终版（SqliteEventStore 已实现后确认无遗漏方法）
  3. CI 三平台（Windows/Linux/macOS）均能构建（Phase 0 验收要求）

---

> **最后更新**：2026-05-16（Sprint 0.5 规划，待开始执行）
