# §六 分阶段开发路线图

> 回到导航：[../../ARCHITECTURE.md](../../ARCHITECTURE.md)
>
> 本章是详细 sprint 级路线图。摘要见根目录 [ROADMAP.md](../../ROADMAP.md)。

## 路线图编排原则

**原则 1：每任务 1-2 周可完成**
模糊的"实现 ECS 系统"会让 DeepSeek 失去方向；具体的"实现 EnttRegistry::create/destroy + 单测覆盖率 ≥ 90%（5 天）"才可执行。

**原则 2：分工标注用三色标签**
- 🟦 **Claude**：架构设计、接口定义、骨架代码、审查、文档
- 🟧 **DeepSeek**：实现、单测、样板代码、算法
- 🟩 **人工**：决策、验证、UX、发布

**原则 3：每阶段必有"验收剧本"**
不能用"完成度 X%"代替验收。验收 = 可运行的端到端剧本。

**原则 4：阶段结束必有里程碑回顾**
每个 Phase 结束写一份 retrospective（写入 `docs/retrospectives/phase-N.md`），调整下个 Phase 的计划。

**原则 5：所有任务以 Visual Studio 2022 + CMake + vcpkg 为默认工具链**
详见 [§八 vs-toolchain](./08-vs-toolchain.md)。

---

## 6.1 Phase 0 — 地基（目标：3 个月，~12 周）

### 阶段目标

**一句话**：能在窗口里看到一个 3D 立方体，鼠标能旋转视角，关闭重开依然能看到 — 同时架构骨架就位，所有"以后要做的事"都有承接的位置。

### 验收剧本（端到端）

1. `git clone` myCad 仓库
2. **Visual Studio 2022**：File → Open → CMake → 选 `vs2022-x64-debug` preset；或命令行 `cmake --preset vs2022-x64-debug && cmake --build --preset vs2022-x64-debug`（一次成功，<10 分钟构建本身，含 vcpkg 依赖下载首次可能 30 min）
3. F5 启动 mycad_app，看到主窗口，3D 视口里有一个红色立方体
4. 鼠标中键拖动 → 视角旋转流畅（≥ 60 fps）
5. 菜单 File → Save As → `box.mycad`
6. 关闭程序，重启，File → Open → `box.mycad` → 立方体重现，视角恢复
7. 控制台无 ERROR 日志，AddressSanitizer 干净
8. CI 三平台 green（Windows VS、Linux GCC、macOS Clang）

### Sprint 划分（12 周 = 6 个双周 sprint）

#### Sprint 0.1（Week 1-2）：项目脚手架

| 任务 | 标签 | 估时 | 验收 |
|---|---|---|---|
| 创建 CMake + vcpkg manifest 项目结构 | 🟦 | 1d | `cmake --preset vs2022-x64-debug` 成功 |
| 创建 `CMakePresets.json`（VS 2022 / Linux / macOS preset） | 🟦 | 1d | VS Open CMake 自动识别 |
| 创建 `tools/vs2022.vsconfig` 一键安装清单 | 🟦 | 0.5d | 新人 import 即可 |
| 配置 CI（GitHub Actions：Windows-2022/Ubuntu-22.04/macOS-13 三平台构建） | 🟦 + 🟩 | 2d | 三平台 green |
| 配置 `.clang-format` / `.clang-tidy` / `.editorconfig` | 🟦 | 1d | VS 自动应用，pre-commit hook 阻止违规 |
| 配置 Catch2 + GoogleBenchmark + Hello World 测试 | 🟦 + 🟧 | 1d | `ctest` + VS Test Explorer 都能发现 |
| 配置 Doxygen + Sphinx 文档骨架 | 🟦 | 1d | `make docs` 输出 HTML |
| 创建 `tools/visualizers/mycad.natvis` 骨架 | 🟦 | 0.5d | VS 调试时基础类型可视化 |
| 提交 ADR-0001 ~ 0009（基于本架构方案） | 🟦 + 🟩 | 1d | docs/adr/ 全部就绪 |
| 创建 `docs/ai-context/CONTEXT-template.md` + `TASKS-template.md` | 🟦 | 0.5d | docs/ai-context/ |

**Sprint 0.1 交付物**：可构建、可测试、有完整文档与 ADR 的空仓库；Visual Studio 一键打开。

#### Sprint 0.2（Week 3-4）：Domain 骨架

| 任务 | 标签 | 估时 | 验收 |
|---|---|---|---|
| 实现核心值对象（Point2D/3D, Vector3D, Transform3D, Axis3D, BoundingBox） | 🟧 | 2d | 单测覆盖 ≥ 90% |
| 实现 Identity 类型（EventId/AggregateId/SketchId/Version） | 🟧 | 1d | 单测 |
| 实现 DomainEvent 基类 + EventTypeRegistry | 🟦 接口 + 🟧 实现 | 2d | 单测 |
| 实现 IGeometryPort 接口 + 全部值对象 | 🟦 | 1d | 编译通过、文档生成 |
| 实现 IEventStore 接口 + EventStream | 🟦 | 1d | 编译通过 |
| 实现 IConstraintSolver 接口（占位，无实现） | 🟦 | 0.5d | 编译通过 |
| 实现 IEntityRegistry 接口 + Component concept | 🟦 | 1d | 编译通过 |
| 写 ADR-0010 起的演进 ADR（本 sprint 的设计决策） | 🟦 | 0.5d | docs 提交 |

**Sprint 0.2 交付物**：所有 Domain 接口编译通过；下游可以并行开发 Adapter。

#### Sprint 0.3（Week 5-6）：Infrastructure Adapter

| 任务 | 标签 | 估时 | 验收 |
|---|---|---|---|
| 实现 InMemoryEventStore（含 subscribe / snapshot） | 🟧 | 3d | 单测覆盖核心场景 ≥ 80% |
| 实现 EnttRegistry（IEntityRegistry 的 EnTT 实现） | 🟧 + 🟦 审查 | 3d | 单测 + ADR-0007 已落地 |
| 实现 OcctGeometryAdapter（仅 makeBox + tessellate + serialize） | 🟧 + 🟦 审查 | 4d | 能创建立方体 + 输出网格 |
| 配置 OCCT 通过 vcpkg 集成（VS 2022 preset 优先） | 🟧 + 🟩 调试 | 2d | 三平台均能构建（VS 上首先验证） |
| CI 加入"Tier A 类型隔离"检查（grep 规则，详见 ADR-0009） | 🟦 + 🟧 | 0.5d | CI 阻断违规 |

**Sprint 0.3 交付物**：能调用 `geometryPort->makeBox(10,10,10)` 拿到 BRepHandle，再 `tessellate` 得到三角网格。

#### Sprint 0.4（Week 7-8）：Application + 渲染

| 任务 | 标签 | 估时 | 验收 |
|---|---|---|---|
| 实现 CommandBus（含 register/send/interceptor） | 🟧 + 🟦 接口审查 | 3d | 单测覆盖 |
| 实现一个 demo CommandHandler：`CreateBoxCommand` | 🟧 | 1d | 单测：发命令 → 事件入 store |
| 实现 IRenderPort 接口（OpenGL 抽象） | 🟦 | 1d | 编译 |
| 实现 OpenGLRenderAdapter（仅渲染三角网格 + 简单 PBR） | 🟧 + 🟦 审查 | 4d | 能在窗口里画一个三角形 |
| 实现基础 Camera（perspective + orbit control） | 🟧 | 2d | 鼠标拖动旋转 |
| Visual Studio 内可 F5 调试到 OcctGeometryAdapter | 🟩 | 0.5d | 断点命中 |

**Sprint 0.4 交付物**：命令到事件到 ECS 到渲染的端到端通路打通；能看到一个三角形；VS 调试体验完整。

#### Sprint 0.5（Week 9-10）：UI + 文件 IO

| 任务 | 标签 | 估时 | 验收 |
|---|---|---|---|
| 配置 Qt 6 通过 vcpkg 集成（含 Qt Visual Studio Tools 扩展） | 🟧 + 🟩 调试 | 2d | 空 Qt 窗口能弹出 |
| 实现主窗口骨架（菜单栏 + 视口 + 状态栏） | 🟧 + 🟦 架构审查 | 3d | UI 出现 |
| 把 OpenGLRenderAdapter 嵌入 QOpenGLWidget | 🟧 | 2d | 视口里画立方体 |
| 实现 SqliteEventStore（IEventStore 的 SQLite 实现） | 🟧 + 🟦 审查 | 4d | 单测 + 能持久化 |
| 实现 .mycad ZIP 容器读写（manifest.json + events.db） | 🟧 | 2d | 单测 |
| 实现 File → Save As / Open 菜单逻辑 | 🟧 | 2d | 端到端：保存 → 关闭 → 重开 → 立方体在 |

**Sprint 0.5 交付物**：完整的"创建 → 保存 → 重开"闭环能跑。

#### Sprint 0.6（Week 11-12）：插件骨架 + 收尾

| 任务 | 标签 | 估时 | 验收 |
|---|---|---|---|
| 实现 IPlugin 接口 + IPluginHost | 🟦 | 1d | 编译 |
| 实现 PluginRegistry（dlopen / LoadLibrary 扫描 + ABI 检查） | 🟧 + 🟦 审查 | 3d | 单测：加载假插件 |
| 创建 `tools/plugin-template/` — 第三方插件 5 分钟跑通的 VS 模板 | 🟦 + 🟧 | 1d | VS Open Folder → F5 调试，菜单出现 |
| AddressSanitizer + LeakSanitizer 跑全套测试 | 🟩 | 1d | 零泄漏、零未定义行为 |
| 写 Phase 0 retrospective | 🟦 + 🟩 | 1d | docs/retrospectives/phase-0.md |
| 准备 v0.0.1 alpha 内部 release（Windows ZIP + Linux AppImage） | 🟩 | 1d | 能给 1-2 个朋友试用 |
| 缓冲 buffer | — | ~1w | 用于消化前面 sprint 的延期 |

**Sprint 0.6 交付物**：插件机制可工作；项目处于"可邀请少数朋友体验"的状态。

### Phase 0 风险与对策

| 风险 | 对策 |
|---|---|
| OCCT 在 macOS 编译困难 | Sprint 0.3 优先在 Windows VS 调通，Linux/macOS 拖到 0.4 末再处理 |
| Qt 6 + OpenGL 4.5 集成 bug 多 | Sprint 0.5 先用 QOpenGLWidget 简单方案 |
| 估时全面超出 | 接受 0.6 留 1-2 周 buffer 是合理设计，不慌 |
| 个人精力波动 | 节假日不安排关键路径任务 |
| Visual Studio CMake 体验回归 | 锁定 VS 2022 LTSC 17.x，升级前先在 dev 分支验证 |

---

## 6.2 Phase 1 — MVP 草图建模（目标：6-9 个月，~24-36 周）

### 阶段目标

**一句话**：完成 [§一 §1.2.3](./01-business.md) 定义的"草图 → 拉伸 → 保存 → 重开 → 修改参数 → 联动更新 → 撤销重做"完整 MVP 剧本，发布开源 v0.1。

### 验收剧本（端到端 9 步）

完全对应 [§一 §1.2.3](./01-business.md) 描述的 9 步剧本。

### Sprint 划分（按主题分组，共 12-18 个双周 sprint）

#### 主题 A：草图核心（5 个 sprint，~10 周）

**Sprint 1.A1（Week 13-14）：Sketch BC 聚合**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 SketchEntity 值对象 | 🟧 | 2d |
| 实现 Sketch 聚合根 + apply | 🟧 + 🟦 接口 | 4d |
| 实现 Sketch 事件类型（10 个） | 🟦 设计 + 🟧 实现 | 3d |
| Sketch 单测覆盖 ≥ 85% | 🟧 | 2d |

**Sprint 1.A2（Week 15-16）：Constraint BC + 求解器集成**
| 任务 | 标签 | 估时 |
|---|---|---|
| 集成 PlaneGCS（vcpkg portfile 或 git submodule） | 🟧 + 🟩 调试 | 3d |
| 实现 PlaneGcsSolver | 🟧 + 🟦 审查 | 4d |
| 实现 SketchConstraintSpec 全部约束类型（15 种） | 🟧 | 2d |
| 求解器 e2e 测试：5 个典型草图场景 | 🟧 | 2d |

**Sprint 1.A3（Week 17-18）：草图 UI 工具栏**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现"进入草图模式"状态机 | 🟧 + 🟦 | 3d |
| 实现绘制工具：直线/圆/矩形/弧 | 🟧 | 4d |
| 实现约束工具栏 | 🟧 | 3d |
| 实现尺寸约束输入框 | 🟧 | 1d |

**Sprint 1.A4（Week 19-20）：草图渲染 + 选择**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 2D 草图实体的 OpenGL 渲染 | 🟧 + 🟦 审查 | 3d |
| 实现拾取（GPU Color Picking 简版） | 🟧 | 3d |
| 实现选中状态高亮 | 🟧 | 2d |
| 实现约束图标渲染 | 🟧 | 2d |

**Sprint 1.A5（Week 21-22）：草图状态反馈**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 DOF 分析器 | 🟧 + 🟦 算法审查 | 4d |
| 实现状态栏提示 + 颜色编码 | 🟧 | 2d |
| 实现"建议添加约束"提示 UI | 🟧 + 🟩 UX | 3d |
| 跨 Sprint 1.A 的 e2e 集成测试 | 🟧 | 1d |

#### 主题 B：特征建模（4 个 sprint，~8 周）

**Sprint 1.B1（Week 23-24）：Feature BC 聚合 + 拉伸**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 FeatureTree 聚合根 | 🟧 + 🟦 接口 | 4d |
| 实现 Feature 事件类型 | 🟦 设计 + 🟧 实现 | 2d |
| 在 OcctGeometryAdapter 中实现 prismaticExtrude | 🟧 + 🟩 OCCT API | 3d |
| 端到端：草图 → 拉伸 → 立方体 | 🟧 | 1d |

**Sprint 1.B2（Week 25-26）：旋转 + 切除特征**
| 任务 | 标签 | 估时 |
|---|---|---|
| OcctGeometryAdapter 实现 revolve | 🟧 | 2d |
| 实现 PadFeature / PocketFeature / RevolveFeature | 🟧 + 🟦 | 3d |
| 实现 Boolean Cut | 🟧 | 3d |
| 特征参数 UI | 🟧 | 2d |

**Sprint 1.B3（Week 27-28）：特征树 UI + 编辑**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现左侧特征树面板（Qt TreeView） | 🟧 + 🟦 | 3d |
| 实现"双击编辑特征参数"流程 | 🟧 | 3d |
| 实现"压制/启用特征"右键菜单 | 🟧 | 1d |
| 实现"删除特征"+ 影响下游的检测 | 🟧 | 3d |

**Sprint 1.B4（Week 29-30）：参数联动 + 重建调度**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 GeometryRebuildSystem | 🟧 + 🟦 架构审查 | 4d |
| 实现"修改草图 → 触发依赖特征重建"事件链 | 🟧 | 3d |
| 实现重建进度条 | 🟧 | 2d |
| 端到端测试：修改草图宽度 → 立方体自动变宽 | 🟧 | 1d |

#### 主题 C：撤销 / 重做 / 持久化（2 个 sprint，~4 周）

**Sprint 1.C1（Week 31-32）：UndoRedoManager**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 OperationScope（命令边界） | 🟦 + 🟧 | 2d |
| 实现 UndoRedoManager（含快路径快照） | 🟧 + 🟦 | 4d |
| 实现 Ctrl+Z / Ctrl+Y 快捷键 | 🟧 | 1d |
| 实现撤销栈 UI 面板 | 🟧 | 2d |
| 端到端测试：撤销/重做 50 步无错 | 🟧 | 1d |

**Sprint 1.C2（Week 33-34）：快照 + 优化**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 SnapshotPolicy | 🟧 + 🟦 | 2d |
| 实现 FeatureTreeSnapshot 序列化（含 OCCT BinTools） | 🟧 | 3d |
| 实现 SQLite EventStore 的快照表 | 🟧 | 2d |
| 性能测试：1000 事件文档打开 < 2s | 🟧 + 🟩 | 2d |
| 优化（Tracy profile + 修热点） | 🟧 + 🟩 | 1d |

#### 主题 D：发布准备（2-3 个 sprint，~4-6 周）

**Sprint 1.D1（Week 35-36）：互操作（最小可用）**
| 任务 | 标签 | 估时 |
|---|---|---|
| 实现 IFormatHandler + StlHandler（导出 STL） | 🟧 + 🟦 | 3d |
| 实现 ObjHandler | 🟧 | 2d |
| 端到端测试：拉伸 → 导出 STL → 用 PrusaSlicer 切片 OK | 🟩 | 1d |
| 实现"截图导出 PNG" | 🟧 | 1d |
| 实现 CHANGELOG.md + 自动 changelog 工具 | 🟦 | 1d |

**Sprint 1.D2（Week 37-38）：稳定性 + 文档**
| 任务 | 标签 | 估时 |
|---|---|---|
| 跑 100 次随机 fuzz 测试，修复发现的 bug | 🟧 + 🟩 | 4d |
| ASan + UBSan 全套测试通过 | 🟩 | 2d |
| 写用户手册（Sphinx，覆盖 9 步剧本） | 🟦 + 🟧 | 3d |
| 录制 5 分钟"快速上手"视频 | 🟩 | 1d |

**Sprint 1.D3（Week 39-40）：v0.1.0 发布**
| 任务 | 标签 | 估时 |
|---|---|---|
| 准备 Linux AppImage 打包 | 🟧 + 🟩 | 2d |
| 准备 Windows ZIP portable 包（含 windeployqt 部署 Qt DLL） | 🟧 + 🟩 | 2d |
| 准备 macOS .app（无签名，标 "未签名" 警告） | 🟧 + 🟩 | 2d |
| GitHub Release v0.1.0 + 写发布博客（中英文） | 🟦 + 🟩 | 2d |
| 发到 HackerNews / r/cpp / r/CAD / 知乎 / V2EX | 🟩 | 1d |
| Phase 1 retrospective | 🟦 + 🟩 | 1d |

### Phase 1 开源发布前 Checklist

- [ ] LICENSE.md 文件存在（LGPL-3.0）
- [ ] CONTRIBUTING.md 写明贡献流程
- [ ] CODE_OF_CONDUCT.md 存在
- [ ] SECURITY.md 写明漏洞报告渠道
- [ ] README 顶部有 GIF demo
- [ ] README 有"快速开始"段落（VS 2022 + Linux + macOS 三选一）
- [ ] CI 三平台 green
- [ ] 所有 P0 测试通过
- [ ] 所有 ADR 已 Accepted
- [ ] CHANGELOG.md 有 v0.1.0 条目
- [ ] 发布博客文章已撰写
- [ ] 准备好回应 1 周内涌入的 issues

### Phase 1 技术挑战预判与应对

| 挑战 | 预判难度 | 应对策略 |
|---|---|---|
| PlaneGCS 集成 | 中 | 预留 1 个 sprint 调试时间；备选自实现 Eigen LM |
| OCCT 拉伸特征的拓扑命名稳定 | 高 | 用事件流 ID 替代 OCCT 拓扑名 |
| 撤销重做与协同事件偏序的兼容 | 中 | Phase 1 先做单用户撤销，vectorClock 字段预留 |
| Qt 信号槽与 ECS 的桥接 | 低 | EventConsumerSystem 中转 |
| macOS OpenGL 弃用 | 中 | Phase 1 接受"macOS 用旧 GL 上下文" |

---

## 6.3 Phase 2 — 完整 CAD（目标：18-24 个月，~36-48 周）

### 阶段目标

**一句话**：从 v0.1 个人玩具升级为 v1.0 可被独立设计师/小工坊真实使用的工具，社区生态初步成型。

### 核心功能目标

1. **装配体**：多零件文件引用、装配约束、爆炸图
2. **工程图**：三视图自动生成、剖视图、尺寸标注、DXF 导出
3. **更多特征**：倒角、圆角、抽壳、扫掠、放样、阵列、镜像
4. **STEP / IGES / DXF 导入导出**：达到 FreeCAD 7 成水平
5. **插件市场基础设施**：插件签名、版本管理、自动更新
6. **AI 功能 v1**：自然语言 → 草图（基础能力）
7. **协同功能预研**：完成事件偏序设计的实战验证（不发布）
8. **稳定性**：MTBF ≥ 8 小时持续使用无崩溃

### 主题划分（不再细化到 sprint，由 Phase 1 retrospective 后调整）

| 主题 | 估时 | 主要工具 | 关键交付 |
|---|---|---|---|
| 装配体 BC | 6 周 | 🟦 + 🟧 | Assembly 聚合、装配约束求解、零件引用 |
| 工程图 BC | 8 周 | 🟦 + 🟧 + 🟩 UX | Drawing 聚合、视图生成、标注、DXF |
| 高级特征（倒角/圆角/抽壳/扫掠/放样） | 8 周 | 🟧 + 🟦 算法选型 | 5 类新特征 |
| 阵列与镜像 | 4 周 | 🟧 | 线性/环形/镜像，含装配级 |
| STEP / IGES / DXF 完善 | 6 周 | 🟧 + 🟩 测试 | 通过 STEP AP242 测试套件 70%+ |
| 插件市场后端 | 4 周 | 🟧 + 🟩 运维 | 简单的插件注册中心（SaaS） |
| 插件签名 + 自动更新 | 3 周 | 🟧 + 🟦 | 数字签名、增量更新 |
| Python 插件支持 | 4 周 | 🟧 + 🟦 | pybind11 嵌入 + .mycadpy 包格式 |
| AI 功能 v1（自然语言→草图） | 6 周 | 🟧 + 🟩 模型选型 | LLM Adapter + 提示词 + 简单 demo |
| AI 功能商业化（计费 / 订阅） | 3 周 | 🟧 + 🟩 业务 | 高级版 / 免费版区分 |
| 性能优化（大装配） | 4 周 | 🟧 + 🟩 profile | 1000 零件装配 60 fps |
| 协同预研（不发布） | 持续 | 🟦 设计 | vectorClock 实战 + ADR |
| v1.0 发布准备 | 4 周 | 🟦 + 🟩 | 安装器、文档、营销 |

### AI 辅助功能引入节点

| 时机 | AI 功能 | 商业策略 |
|---|---|---|
| Phase 2 Week ~30 | 自然语言 → 草图原语 | 内置免费版 |
| Phase 2 Week ~36 | 特征智能命名 | 免费版 |
| Phase 2 Week ~42 | 复杂自然语言 → 完整草图 | 高级版 99 元/月 |

### 生态建设里程碑

| 里程碑 | 目标时间 | 关键动作 |
|---|---|---|
| 第一个外部 PR 合并 | Phase 2 W12 | 主动引导 issue → PR |
| 第一个第三方插件发布 | Phase 2 W24 | 自己写示例 + 给 1-2 个开发者 1-on-1 帮助 |
| 插件市场上线 | Phase 2 W36 | 5 个免费 + 2 个付费插件作为种子 |
| 第一个付费用户（高级 AI 订阅） | Phase 2 W42 | 配套定价页 + 支付集成 |
| 首个企业 PoC（私有化部署） | Phase 2 W44+ | 主动接触 1-2 家潜在客户 |

---

## 6.4 Phase 3 — 向 CAM/CAE 扩展（24+ 个月）

### 阶段目标

**一句话**：从单一 CAD 工具进化为"统一设计平台"，CAM 与 CAE 通过插件生态接入；同时验证架构对长期演进的承受力。

### 核心策略

**不要试图自己造 CAM/CAE 内核** — 那是 5-10 人年的工程。

正确做法：
1. **myCad 提供完整的扩展点**（Component 类型注册、System 注入、文件格式扩展）
2. **找愿意合作的第三方** — CAM 用现成的开源核（如 LinuxCNC、CAMotics）；CAE 用 OpenFOAM、Calculix
3. **myCad 的价值是"统一上下文"** — 用户在同一软件里走完 CAD → CAM → CAE 流程

### ECS 扩展路径示例

#### 添加 CAM 能力（不需要改架构）

```cpp
// 1. 第三方插件注册新 Component 类型
ecs->registerComponentType<CAMToolpathComponent>();
ecs->registerComponentType<CAMStockComponent>();

// 2. 注册新 System
class ToolpathGenerationSystem : public ISystem {
    void update() override {
        auto view = ecs.view<BRepGeometryComponent, CAMToolpathRequestTag>();
        for (auto&& [eid, brep, _] : view.each()) {
            auto toolpath = generatePocketToolpath(brep);
            ecs.add(eid, CAMToolpathComponent{std::move(toolpath)});
            ecs.remove<CAMToolpathRequestTag>(eid);
        }
    }
};
plugin->host().registerSystem(
    std::make_unique<ToolpathGenerationSystem>(),
    SystemPhase::UserDefined1);

// 3. 注册新文件格式（G-Code 导出）
plugin->host().formatRegistry().registerHandler(
    std::make_unique<GCodeHandler>());

// 4. 注册新事件类型
class ToolpathGenerated : public DomainEvent { /* ... */ };
MYCAD_REGISTER_EVENT(ToolpathGenerated)

// 5. 注册新 UI 贡献
plugin->host().ui().registerToolbar(
    "CAM", { /* buttons */ });
```

**关键观察**：上述 5 步**没有触动 myCad 内核任何代码**。这是 Phase 0 在架构上做对了的红利。

#### 添加 CAE 能力（同理）

```cpp
ecs->registerComponentType<FEAMeshComponent>();
ecs->registerComponentType<BoundaryConditionComponent>();
ecs->registerComponentType<MaterialPropertyComponent>();
ecs->registerComponentType<FEAResultComponent>();

class MeshGenerationSystem : public ISystem { /* 调用 Gmsh / NetGen */ };
class SolverInterfaceSystem : public ISystem { /* 调用 Calculix / OpenFOAM */ };
class ResultVisualizationSystem : public ISystem { /* 渲染应力云图 */ };
```

### 架构演进策略

#### 必然需要重构的地方

| 区域 | 触发条件 | 重构方向 | 缓解 |
|---|---|---|---|
| **EventStore** | 单文档事件 > 10 万 | 分片：按 BC 拆分多个 SQLite | EventStore 接口稳定 |
| **OCCT Adapter** | 渲染需要更细的 OCCT 拓扑信息 | 增加 IGeometryPort 方法 | 加方法不破坏既有 |
| **Component 内存布局** | ECS 实体数 > 10 万有 GC 压力 | 引入 EnTT pool 自定义分配器 | 接口零变化 |
| **渲染层** | macOS OpenGL 真正不可用 | 引入 wgpu 后端 | IRenderPort 接口稳定 |

#### 大概率不需要重构的地方（架构红利）

| 区域 | 为什么稳定 |
|---|---|
| Domain 聚合根 | 业务模型变化慢，DDD 正是为此 |
| 事件类型与 schema | FlatBuffers schema evolution 良好 |
| 插件接口 | 双 ABI 边界设计有效 |
| CommandBus | 抽象层次正确 |

#### 重构最小化原则

1. **任何需要破坏接口的重构必须先写 ADR**
2. **重构与功能开发严格分离**：单 PR 只做一类事
3. **保留旧接口至少 2 个版本**：Deprecate → Warn → Remove
4. **重大重构前发 RFC**：在 GitHub Discussions 给社区 2 周窗口反馈

---

## 6.5 路线图总览（一图看全 24 个月）

```
Phase 0  地基 (3m)              Phase 1  MVP (6-9m)            Phase 2  完整 CAD (18-24m)        Phase 3  CAM/CAE 扩展
========================        =====================         ==============================    ====================
Sprint 0.1 脚手架               草图核心 (5 sprint)             装配体 (6w)                       第三方 CAM 接入
Sprint 0.2 Domain 骨架          特征建模 (4 sprint)             工程图 (8w)                       第三方 CAE 接入
Sprint 0.3 Adapter              撤销持久化 (2 sprint)           高级特征 (8w)                     架构演进
Sprint 0.4 渲染端到端           发布准备 (3 sprint)             阵列镜像 (4w)                     云协同上线
Sprint 0.5 UI + 文件 IO                                        STEP/IGES/DXF 完善 (6w)
Sprint 0.6 插件骨架                                            插件市场 (4w + 3w)
                                                              Python 插件 (4w)
                                                              AI v1 (6w)
                                                              AI 商业化 (3w)
                                                              性能优化 (4w)
                                                              v1.0 发布 (4w)

里程碑：                       里程碑：                         里程碑：
- 看到 3D 立方体                - v0.1 公开发布                  - v1.0 公开发布
- 架构骨架就位                  - 第一个 Issue                   - 第一个企业 PoC
- 内部 alpha                   - GitHub 100 Star                - GitHub 1000 Star
- VS + Linux + macOS 三平台 CI                                   - 第一个付费用户
```

---

> **最后修订**：2026-05（首次拆分自 ARCHITECTURE.md，Visual Studio 工具链已纳入）
