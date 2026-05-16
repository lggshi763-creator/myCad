# Sprint 0.4 — Application + 渲染（Week 7-8，2026-05-18 ~ 05-29）

> **工具分工**：🟦 Claude Code（全程直接实现）·🟩 人工（决策/验证）
>
> **每日编码时间上限**：2-3 小时。Day 1-10 覆盖 10 个工作日。
>
> **参考**：[ADR-0005](../adr/ADR-0005-opengl-not-vulkan.md) · [05-code-skeletons.md §5.7](../architecture/05-code-skeletons.md) · [06-roadmap.md §Sprint 0.4](../architecture/06-roadmap.md)

---

## 阶段目标（一句话）

**命令 → 事件 → ECS → 渲染的端到端通路打通**：`CreateBoxCommand` 经 CommandBus → OcctGeometryAdapter → InMemoryEventStore → EnttRegistry → OpenGLRenderAdapter，最终在 Qt 窗口里看到一个可用鼠标旋转的立方体。

---

## 前置确认（Sprint 启动时已满足）

- [x] Qt 6 已通过 vcpkg 安装（`Qt6Core.lib`、`Qt6OpenGL.lib` 等本地存在）
- [x] `src/ui/CMakeLists.txt` 已链接 `Qt6::OpenGL Qt6::OpenGLWidgets`
- [x] Sprint 0.3 全量 157 tests PASS，OcctGeometryAdapter / InMemoryEventStore / EnttRegistry 均可用

---

## Sprint 验收清单

- [ ] **V1** `CommandBus::send<CreateBoxCommand>` 端到端：事件落入 InMemoryEventStore，ECS 有对应 entity
- [ ] **V2** `mycad_app_tests` 全部 PASS（CommandBus 单元测试 + 集成测试）
- [ ] **V3** Qt 窗口打开，渲染出立方体 wireframe 或实体（OpenGL 三角形可见）
- [ ] **V4** 鼠标左键拖动 → 轨道旋转，滚轮 → 缩放
- [ ] **V5** VS F5 可断点命中 `OcctGeometryAdapter::makeBox`（调试体验完整）
- [ ] **V6** `src/domain/` 零外部依赖仍成立（CI domain-isolation job 持续绿）

---

## 任务总览

| # | 任务 | 估时 | 验收 |
|---|---|---|---|
| T1 | CommandBus 接口 + CommandContext + CommandError | 1d | 编译通过，零外部依赖 |
| T2 | CommandBus 实现（type-erased dispatch）+ 单元测试 | 2d | V2 CommandBus 部分 ✓ |
| T3 | CreateBoxCommand + CreateBoxCommandHandler + 集成测试 | 1d | V1 ✓ |
| T4 | IRenderPort 接口 + OpenGLRenderAdapter scaffold | 1d | 编译通过 |
| T5 | OpenGL 第一个三角形（shader + VAO/VBO + draw loop） | 2d | 屏幕上可见一个彩色三角形 |
| T6 | 网格上传：TriangleMesh → VAO，渲染立方体 | 1d | V3 ✓ |
| T7 | MainWindow + QOpenGLWidget + Camera（perspective + orbit） | 2d | V4 ✓ |
| T8 | 端到端连通 + VS 调试配置 + Sprint 收尾 | 1d | V5 V6 ✓ |

**估时合计**：11d × ~2.5h = **~27.5h**（含 buffer）

---

## 架构决策（Sprint 内有效）

| 决策 | 理由 |
|---|---|
| `CommandResult` = `std::expected<void, CommandError>`（Sprint 0.4 简化版） | 所有命令走 CQRS，结果通过 EventStore/ECS 查询，不需要命令返回数据 |
| `CommandContext` 用 `std::string userId/operationId`，不引入 UserId 值对象 | 正式 UserId 类型在身份 BC 落地时再加，Sprint 0.4 提前抽象过度 |
| IRenderPort 放在 `src/domain/shared/include/mycad/domain/` | 与 IGeometryPort、IEntityRegistry 同层：domain 纯接口，零 OpenGL 依赖 |
| OpenGLRenderAdapter 放在 `src/infrastructure/rendering/` | 与 OCCT、EnTT 适配器同层，遵循 ADR-0004 模式 |
| 使用 `QOpenGLWidget` 而非独立 GLFW 窗口 | Qt 已安装；Sprint 0.5 Qt UI 直接复用；无需再引入 glfw 依赖 |
| Camera 数学（MVP 矩阵）用 Eigen3（已在 vcpkg） | 不重复造轮子；Eigen 已有 `lookAt` / `perspective` 等实用函数 |
| Shader 用 GLSL 420（Core Profile） | 与 ADR-0005 OpenGL 4.5 DSA 对齐；DSA API 在 T5 使用 |

---

## 目录结构（Sprint 0.4 新增）

```
src/
├── domain/shared/include/mycad/domain/
│   └── IRenderPort.hpp                          ← T4（纯接口，零 OpenGL）
├── application/
│   ├── include/mycad/application/
│   │   ├── CommandBus.hpp                       ← T1/T2
│   │   ├── ICommandHandler.hpp                  ← T1
│   │   ├── CommandContext.hpp                   ← T1
│   │   └── CommandError.hpp                     ← T1
│   ├── commands/
│   │   └── include/mycad/application/commands/
│   │       └── CreateBoxCommand.hpp             ← T3
│   ├── handlers/
│   │   ├── include/mycad/application/handlers/
│   │   │   └── CreateBoxCommandHandler.hpp      ← T3
│   │   └── CreateBoxCommandHandler.cpp          ← T3
│   ├── CommandBus.cpp                           ← T2
│   └── CMakeLists.txt                           ← 更新
├── infrastructure/rendering/
│   ├── include/mycad/infrastructure/
│   │   └── OpenGLRenderAdapter.hpp              ← T4/T5/T6（PIMPL）
│   └── OpenGLRenderAdapter.cpp                  ← T5/T6（所有 GL 调用在此）
└── ui/
    ├── include/mycad/ui/
    │   ├── MainWindow.hpp                       ← T7
    │   └── ViewportWidget.hpp                   ← T7
    ├── MainWindow.cpp                           ← T7
    ├── ViewportWidget.cpp                       ← T7（QOpenGLWidget，持有 adapter）
    └── CMakeLists.txt                           ← 更新

tests/
├── application/
│   └── commandbus_test.cpp                      ← T2/T3
└── CMakeLists.txt                               ← 更新（新增 mycad_app_tests）
```

---

## 每日任务分解

---

### Day 1（~2.5h）：T1 — CommandBus 接口设计

> **前置**：无（纯接口，零外部依赖）

#### 新建文件

**`src/application/include/mycad/application/CommandError.hpp`**：
```cpp
enum class CommandErrorKind {
    HandlerNotFound, AggregateLoadFailed,
    BusinessRuleViolated, ConcurrencyConflict, HandlerThrew,
};
struct CommandError {
    CommandErrorKind kind;
    std::string      message;
};
using CommandResult = std::expected<void, CommandError>;
```

**`src/application/include/mycad/application/CommandContext.hpp`**：
```cpp
struct CommandContext {
    std::string userId;      // opaque，Sprint 0.4 简化
    std::string operationId; // opaque，Sprint 0.4 简化
    std::string source;      // "test" / "ui" / "plugin:xyz"
};
```

**`src/application/include/mycad/application/ICommandHandler.hpp`**：
```cpp
template <typename C>
concept Command = std::is_class_v<C> && std::is_move_constructible_v<C>;

template <Command C>
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    virtual CommandResult handle(const C& cmd, const CommandContext& ctx) = 0;
};
```

**`src/application/include/mycad/application/CommandBus.hpp`**：
```cpp
class CommandBus {
public:
    explicit CommandBus(std::shared_ptr<domain::IEventStore> store);
    ~CommandBus();

    template <Command C>
    void registerHandler(std::shared_ptr<ICommandHandler<C>> handler);

    template <Command C>
    CommandResult send(C cmd, CommandContext ctx = {});

    [[nodiscard]] std::shared_ptr<domain::IEventStore> eventStore() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

> Impl 中用 `unordered_map<type_index, HandlerEntry>`，`HandlerEntry` 含类型擦除的 invoker。
> `registerHandler` / `send` 模板体在 .hpp，Impl 的 ctor/dtor 在 .cpp（避免 Impl 前置声明问题）。

#### 更新 CMakeLists.txt

`src/application/CMakeLists.txt`：
- 升级 `cxx_std_20` → `cxx_std_23`（`<expected>` 需要）
- 添加 source 文件（暂时仍 placeholder.cpp，Day 2 换 CommandBus.cpp）
- `PUBLIC` 链接 `mycad::domain`

#### 验收

- [ ] 四个头文件编译通过

---

### Day 2-3（~2.5h × 2）：T2 — CommandBus 实现 + 单元测试

> **前置**：T1 完成

#### CommandBus.cpp

```cpp
struct CommandBus::Impl {
    struct HandlerEntry {
        std::shared_ptr<void> handler;
        std::function<CommandResult(const void*, const CommandContext&)> invoker;
    };
    std::unordered_map<std::type_index, HandlerEntry> handlers;
    std::shared_ptr<domain::IEventStore>              store;
    std::mutex                                        mu;
};

// registerHandler<C>:
//   impl_->handlers[type_index(typeid(C))] = {
//       handler,
//       [h=handler](const void* p, const CommandContext& ctx) {
//           return h->handle(*static_cast<const C*>(p), ctx);
//       }
//   };

// send<C>:
//   lock_guard → find type_index → not found → HandlerNotFound
//   found → invoker(&cmd, ctx) → propagate result
```

#### 测试（`tests/application/commandbus_test.cpp`）

```
TEST_CASE "CommandBus - send with no handler returns HandlerNotFound"
TEST_CASE "CommandBus - register and send dispatches to handler"
TEST_CASE "CommandBus - handler receives correct command value"
TEST_CASE "CommandBus - handler receives CommandContext"
TEST_CASE "CommandBus - two commands register two handlers independently"
TEST_CASE "CommandBus - handler returning error propagates to caller"
TEST_CASE "CommandBus - eventStore() returns injected store"
```

#### 新增测试 target

`tests/CMakeLists.txt` 加 `mycad_app_tests`：
- sources: `application/commandbus_test.cpp`（后续加 `application/createbox_test.cpp`）
- links: `Catch2::Catch2WithMain mycad::application mycad::compile_options`

#### 验收

- [ ] 所有 CommandBus 单元测试 PASS
- [ ] 无 `#include <entt/...>` / `#include <opencascade/...>` 在 application 层

---

### Day 4（~2.5h）：T3 — CreateBoxCommand + Handler + 集成测试

> **前置**：T2 完成；OcctGeometryAdapter、EnttRegistry、InMemoryEventStore 均可用

#### CreateBoxCommand.hpp

```cpp
struct CreateBoxCommand {
    double       dx{1.0}, dy{1.0}, dz{1.0};
    domain::AggregateId targetId;  // 要绑定的聚合 ID，由 UI 生成
};
```

#### CreateBoxCommandHandler

```cpp
class CreateBoxCommandHandler final : public ICommandHandler<CreateBoxCommand> {
public:
    CreateBoxCommandHandler(
        std::shared_ptr<infrastructure::OcctGeometryAdapter> geom,
        std::shared_ptr<infrastructure::EnttRegistry>        ecs,
        std::shared_ptr<domain::IEventStore>                 store);

    CommandResult handle(const CreateBoxCommand& cmd,
                         const CommandContext&   ctx) override;
private:
    // 逻辑：
    // 1. geom->makeBox(dx, dy, dz) → BRepHandle
    // 2. ecs->create() → entity
    // 3. ecs->emplace<BRepHandle>(entity, handle)
    // 4. store->appendOne(targetId, Version{0}, BoxCreatedEvent{...})
    // 5. 返回 CommandResult{}（成功）
};
```

注意：Handler 在 application 层，可以 `#include` infrastructure 头（OcctGeometryAdapter、EnttRegistry）。这符合六边形架构——application 是内六边形的驱动侧，infrastructure 是被驱动侧。

#### BoxCreatedEvent（domain 事件）

```cpp
// src/domain/shared/include/mycad/domain/events/BoxCreatedEvent.hpp
struct BoxCreatedEvent final : DomainEvent {
    MYCAD_DOMAIN_EVENT("BoxCreatedEvent")
    double dx, dy, dz;
    domain::BRepHandle handle;
};
```

#### 集成测试

```
TEST_CASE "CreateBoxCommandHandler - success path"
  bus.registerHandler(handler)
  bus.send(CreateBoxCommand{10, 10, 10, id})
  CHECK store 有 1 个事件，typeName == "BoxCreatedEvent"
  CHECK ecs->size() == 1

TEST_CASE "CreateBoxCommandHandler - zero size returns BusinessRuleViolated"
  bus.send(CreateBoxCommand{0, 10, 10, id})
  CHECK error.kind == CommandErrorKind::BusinessRuleViolated
```

#### 验收（V1）

- [ ] 集成测试 PASS：命令 → 事件落入 store，ECS 有 entity

---

### Day 5（~2.5h）：T4 — IRenderPort 接口 + OpenGLRenderAdapter scaffold

> **前置**：T3 完成

#### IRenderPort.hpp（domain 层）

```cpp
// src/domain/shared/include/mycad/domain/IRenderPort.hpp
class IRenderPort {
public:
    virtual ~IRenderPort() = default;
    IRenderPort(const IRenderPort&)            = delete;
    IRenderPort& operator=(const IRenderPort&) = delete;

    /// @brief 上传或替换 BRepHandle 对应的三角网格。
    virtual void uploadMesh(domain::BRepHandle handle,
                            const domain::TriangleMesh& mesh) = 0;

    /// @brief 移除 BRepHandle 对应的渲染数据。
    virtual void removeMesh(domain::BRepHandle handle) noexcept = 0;

    /// @brief 设置视图矩阵（4×4 列主序 float）。
    virtual void setViewMatrix(const float* mat4) noexcept = 0;

    /// @brief 设置投影矩阵（4×4 列主序 float）。
    virtual void setProjectionMatrix(const float* mat4) noexcept = 0;

    /// @brief 清屏 + 绘制所有已上传网格。
    virtual void render() = 0;

protected:
    IRenderPort() = default;
};
```

#### OpenGLRenderAdapter.hpp（infrastructure，PIMPL）

```cpp
// src/infrastructure/rendering/include/mycad/infrastructure/OpenGLRenderAdapter.hpp
// 只 include domain 和标准库，零 OpenGL 头文件
#include <mycad/domain/IRenderPort.hpp>

class OpenGLRenderAdapter final : public domain::IRenderPort {
public:
    OpenGLRenderAdapter();
    ~OpenGLRenderAdapter() override;
    void uploadMesh(...) override;
    void removeMesh(...) noexcept override;
    void setViewMatrix(const float*) noexcept override;
    void setProjectionMatrix(const float*) noexcept override;
    void render() override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

#### OpenGLRenderAdapter.cpp scaffold

所有 `#include <GL/gl.h>` / `<QOpenGLFunctions_4_5_Core.h>` 仅在此文件。

Impl 包含：
```cpp
struct MeshEntry {
    GLuint vao{0}, vbo{0}, ebo{0};
    GLsizei indexCount{0};
};
struct Impl {
    QOpenGLFunctions_4_5_Core* gl{nullptr};  // 从 QOpenGLContext 获取
    GLuint shaderProgram{0};
    std::unordered_map<uint64_t, MeshEntry> meshes;
    float viewMat[16]{};
    float projMat[16]{};
};
```

> `gl` 指针由 `ViewportWidget::initializeGL()` 在 GL context 就绪后注入（Day 7）。

#### 更新 infrastructure CMakeLists

加入 `rendering/OpenGLRenderAdapter.cpp`，PUBLIC include `rendering/include`，
PRIVATE 链接 `Qt6::OpenGL`（QOpenGLFunctions_4_5_Core 所在头）。

#### 验收

- [ ] 编译通过（即使 Impl 里 gl 为 nullptr 也不崩溃）
- [ ] `grep -r "#include.*GL/" src/infrastructure/rendering/include/` 输出为空

---

### Day 6-7（~2.5h × 2）：T5 — OpenGL 第一个三角形

> **前置**：T4 完成，Qt context 可用

#### Shader（GLSL 420 Core Profile）

**vertex.glsl** → **fragment.glsl** 内联为 C++ `const char*` 字符串字面量（避免资源文件依赖）：

```glsl
// vertex（420 core）
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
out vec3 vNormal;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); vNormal = aNormal; }

// fragment（420 core）
in vec3 vNormal;
out vec4 FragColor;
void main() {
    float light = max(dot(normalize(vNormal), vec3(0.6,0.8,0.4)), 0.1);
    FragColor = vec4(vec3(0.4, 0.7, 1.0) * light, 1.0);
}
```

#### OpenGLRenderAdapter::Impl::initialize()

```
1. glCreateProgram / glCreateShader / glShaderSource / glCompileShader / glLinkProgram
2. 创建一个单位三角形 MeshEntry（调试用，不绑定 BRepHandle）作为初始化验证
3. glCreateVertexArrays(1, &vao)  — DSA
4. glCreateBuffers(1, &vbo)
5. glNamedBufferStorage(vbo, ...)
6. glVertexArrayVertexBuffer / glVertexArrayAttribFormat / glEnableVertexArrayAttrib
```

#### render() 逻辑

```
glClearColor(0.15, 0.15, 0.15, 1.0);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glUseProgram(shaderProgram);
for each MeshEntry:
    mat4 mvp = proj * view;   // 手动矩阵乘（或 Eigen）
    glUniformMatrix4fv(uMVP, ...)
    glBindVertexArray(vao)
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0)
```

#### 验收

- [ ] QOpenGLWidget 显示出灰蓝色背景 + 蓝色三角形

---

### Day 7 续（~1h）：T6 — 网格上传，渲染立方体

#### uploadMesh()

```
1. 若 handle 已存在 → glDeleteVertexArrays/Buffers 先清除
2. glCreateVertexArrays + glCreateBuffers (DSA)
3. glNamedBufferStorage(vbo, vertices)
4. glNamedBufferStorage(ebo, indices)
5. 绑定 attrib：location 0 → xyz(float×3)，location 1 → normal（后续加，Sprint 0.4 可先全零）
6. 存入 meshes[handle.id]
```

#### 验收（V3）

- [ ] `bus.send(CreateBoxCommand{2,3,4,id})` → tessellate → uploadMesh → 立方体可见

---

### Day 8-9（~2.5h × 2）：T7 — MainWindow + QOpenGLWidget + Camera

> **前置**：T5/T6 完成

#### Camera（`src/infrastructure/rendering/include/mycad/infrastructure/Camera.hpp`）

```cpp
struct Camera {
    float azimuth{0.f};    // 水平角（弧度）
    float elevation{0.4f}; // 垂直角（弧度）
    float radius{8.f};     // 距目标距离
    float fovY{45.f};      // 度
    Eigen::Vector3f target{0.f, 0.f, 0.f};

    Eigen::Matrix4f viewMatrix() const;       // lookAt
    Eigen::Matrix4f projMatrix(float aspect) const; // perspective
    void orbit(float dAz, float dEl) noexcept;
    void zoom(float delta) noexcept;
};
```

#### ViewportWidget（QOpenGLWidget 子类）

```cpp
class ViewportWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit ViewportWidget(QWidget* parent = nullptr);
    void setRenderAdapter(std::shared_ptr<infrastructure::OpenGLRenderAdapter>);

protected:
    void initializeGL() override;    // 注入 gl，调用 adapter->initialize()
    void resizeGL(int w, int h) override;  // 更新 projection
    void paintGL() override;         // setView/Proj → adapter->render()
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    std::shared_ptr<infrastructure::OpenGLRenderAdapter> adapter_;
    infrastructure::Camera camera_;
    QPoint lastMousePos_;
};
```

#### MainWindow

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(
        std::shared_ptr<application::CommandBus> bus,
        std::shared_ptr<infrastructure::OpenGLRenderAdapter> renderer,
        QWidget* parent = nullptr);
private:
    ViewportWidget* viewport_;
    // Sprint 0.5 加菜单栏 / 工具栏
};
```

#### main.cpp（`src/ui/main.cpp`）

```cpp
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    auto store    = std::make_shared<infrastructure::InMemoryEventStore>();
    auto geom     = std::make_shared<infrastructure::OcctGeometryAdapter>();
    auto ecs      = std::make_shared<infrastructure::EnttRegistry>();
    auto renderer = std::make_shared<infrastructure::OpenGLRenderAdapter>();
    auto bus      = std::make_shared<application::CommandBus>(store);

    auto handler = std::make_shared<application::CreateBoxCommandHandler>(
        geom, ecs, store);
    // handler 还持有 renderer：createBox 后自动 uploadMesh
    bus->registerHandler<application::CreateBoxCommand>(handler);

    // 发一条演示命令
    bus->send(application::CreateBoxCommand{2.0, 3.0, 4.0, domain::AggregateId{}});

    MainWindow win(bus, renderer);
    win.resize(1280, 720);
    win.show();
    return app.exec();
}
```

#### 更新 ui/CMakeLists.txt

- 加 `main.cpp`、`MainWindow.cpp`、`ViewportWidget.cpp`
- 加 `add_executable(mycad_app ...)` → 链接 `mycad::ui mycad::application mycad::infrastructure`
- 加 Eigen3：`find_package(Eigen3 REQUIRED)` → `target_link_libraries(... Eigen3::Eigen)`

#### 验收（V3 V4）

- [ ] `cmake --build --preset local-debug` 后能运行 `mycad_app.exe`
- [ ] 窗口 1280×720，背景深灰，立方体蓝色实体
- [ ] 鼠标左键拖动旋转，滚轮缩放

---

### Day 10（~2h）：T8 — 端到端连通 + VS 调试 + Sprint 收尾

1. **VS 调试配置**：在 `launch.vs.json`（或 CMakeSettings.json）加 `mycad_app` 的 debug target，确认 F5 能命中 `OcctGeometryAdapter::makeBox` 断点

2. **CI 更新**：更新 `tests/CMakeLists.txt` 加 `mycad_app_tests`，确认 CI domain-isolation 仍绿

3. **全量验证**：
   ```powershell
   cmake --build --preset local-debug --parallel
   ctest --preset local-debug --output-on-failure
   ```

4. **Sprint 收尾**：勾选 V1-V6，写 devlog，push + tag `sprint-0.4-done`

---

## 本 Sprint 可能产生的 ADR

| ADR 编号 | 主题 | 触发条件 |
|---|---|---|
| ADR-0015 | OpenGL context 管理策略（QOpenGLWidget vs QWindow + QOffscreenSurface） | 若 QOpenGLWidget 在 VS 调试下有 context 泄漏问题 |
| ADR-0016 | Camera 数学库选型（Eigen3 vs glm vs 手写） | 若 Eigen3 编译在某平台有问题 |

---

## 给下个 Sprint 的预热（Sprint 0.5）

- **主题**：UI 完整骨架 + 文件 IO（菜单栏 / SqliteEventStore / .mycad 容器）
- **Sprint 0.5 启动前需要确认**：
  1. `mycad_app.exe` F5 可调试（V5 已满足）
  2. 阅读 `docs/architecture/08-vs-toolchain.md §8.2.5`（Qt AUTOMOC 配置）
  3. 确认 `qttools`（`lupdate` / `lrelease`）在 vcpkg 已安装

---

> **最后更新**：2026-05-16（Sprint 0.4 启动，Claude Code 生成）
