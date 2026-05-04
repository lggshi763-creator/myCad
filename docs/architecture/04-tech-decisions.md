# §四 关键技术决策清单

> 本章是各技术选型的"账本"。配套深入论证见各 [ADR](../adr/)。
>
> **新增列说明**（相比初版）：
> - **自研估时**：如果未来必须自研替代，预估的最小工程量（人月）
> - **自研触发条件**：什么信号出现时应启动自研评估
> - **抽象隔离层**：当前通过哪个接口隔离第三方实现
> 详细自研策略见 [§九 第三方依赖与自研路径](./09-self-host-strategy.md)。

## 4.1 决策汇总表

### 4.1.1 核心架构与协议

| # | 决策项 | 推荐选型 | 核心理由（一句话） | 替代方案 | 风险点 | 抽象隔离层 | 自研估时 | 自研触发 |
|---|---|---|---|---|---|---|---|---|
| 1 | **几何内核** | OpenCASCADE 7.7+ | CAD 行业唯一开源工业级 BRep 内核 | CGAL（学术）、Open NURBS、自研 | LGPL 商业边界；C++03 风格 | `IGeometryPort` | 50-100 月 | OCCT 停更 / 协议变化 |
| 2 | **GUI 框架** | Qt 6.5+ LTS | C++ 桌面 GUI 事实标准 | wxWidgets、Dear ImGui、Slint | LGPL 动态链接限制 | UI 层隔离 | 40-80 月（替换 Slint：12 月） | Qt 商业政策变化 |
| 3 | **构建系统** | CMake 3.25+ | Visual Studio + vcpkg + OCCT 全部原生支持 | Bazel、Meson、Premake | 语法历史包袱 | — | — | — |
| 4 | **包管理** | vcpkg（Manifest 模式） | OCCT/Qt portfile 质量最高 | Conan、git submodule | baseline 升级回归 | — | — | — |
| 5 | **3D 渲染 API** | OpenGL 4.5 Core + DSA | CAD 渲染瓶颈不在 draw call | Vulkan、D3D、Metal | macOS 长期弃用 | `IRenderPort` | 12-24 月（+ wgpu Adapter：6 月） | macOS 不可用 / Web 端需求 |
| 6 | **协议** | LGPL-3.0-or-later | 与 OCCT 兼容 + 允许商业插件 | MIT、GPL、AGPL | 闭源衍生限制 | — | — | — |

### 4.1.2 IDE 与开发环境

| # | 决策项 | 推荐选型 | 核心理由 | 替代方案 | 风险点 | 抽象隔离层 | 备注 |
|---|---|---|---|---|---|---|---|
| 7 | **★ 主 IDE** | **Visual Studio 2022 Community** | C++ 智能感知最强；CMake/vcpkg/Qt/Catch2 全部一等支持；调试器 Windows 最佳；个人开发者免费 | VSCode + clangd、CLion、Qt Creator | Windows 偏向（CI 必须三平台覆盖） | CMake + CMakePresets.json | 详见 [§八 vs-toolchain](./08-vs-toolchain.md)、[ADR-0008](../adr/ADR-0008-visual-studio-toolchain.md) |
| 8 | **跨平台支持** | Linux：VSCode/CLion；macOS：CLion/Xcode | 项目"主推荐 VS，不强制 IDE" | — | 工具链不一致风险 | CMakePresets 多 preset | CI 强制三平台 green |
| 9 | **VS 必装扩展** | Qt VS Tools、Test Adapter for Catch2、Clang Power Tools、Markdown Editor v2 | 见 §8.2.3 | — | — | — | 写入 `.vsconfig` 一键 import |
| 10 | **AI 行内补全** | GitHub Copilot | Visual Studio 原生集成 | Tabnine、Cursor | 非项目强制 | — | 与 Claude Code/DeepSeek 互补：Copilot 行内、Claude 设计、DeepSeek 实现 |

### 4.1.3 核心库（Tier A — 含 Port 抽象）

| # | 决策项 | 推荐选型 | 核心理由 | 替代方案 | 风险点 | 抽象隔离层 | 自研估时 | 自研触发 |
|---|---|---|---|---|---|---|---|---|
| 11 | **ECS 框架** | EnTT 3.13+ | C++ 圈最快、API 最现代 | flecs、自研 | 单一维护者 | `IEntityRegistry` | 3-6 月 | EnTT 维护断档 |
| 12 | **约束求解器** | PlaneGCS（FreeCAD 提取） | LGPL；FreeCAD 验证 10 年 | SolveSpace（GPL）、Eigen LM | 提取需适配工作 | `IConstraintSolver` | 6-12 月 | PlaneGCS 集成失败 |
| 13 | **EventStore 后端** | SQLite（Phase 1）→ PostgreSQL（协同） | 单文件即 .mycad 容器 | RocksDB、自实现 | 大文档性能 | `IEventStore` | 6-12 月 | SQLite 触瓶颈（极少） |
| 14 | **Python 嵌入** | CPython 3.11 + pybind11 | C++ 圈事实标准 | nanobind、Boost.Python | GIL 处理复杂 | `IPluginHost` Python adapter | 不自研 | — |

### 4.1.4 工具库（Tier B — 直接使用，无抽象层）

| # | 决策项 | 推荐选型 | 核心理由 | 替代方案 | 风险点 | 自研估时 | 备注 |
|---|---|---|---|---|---|---|---|
| 15 | **线性代数** | Eigen 3.4+ | C++ 数值事实标准 | Blaze、Armadillo | 编译时间 | 50+ 月（不自研） | — |
| 16 | **序列化** | FlatBuffers 23.x | 反序列化零拷贝 | Protobuf、Cap'n Proto | Schema 工具链 | 不自研 | 工具型库，可换同类 |
| 17 | **配置文件** | nlohmann::json + tomlplusplus | JSON manifest + TOML 配置 | YAML、INI | JSON 注释支持差 | 不自研 | — |
| 18 | **测试框架** | Catch2 v3 | 现代 C++ 风格；VS Test Adapter 直接发现 | doctest、GoogleTest | v2→v3 不兼容 | 不自研 | — |
| 19 | **Mock** | trompeloeil | 与 Catch2 自然集成 | GoogleMock、FakeIt | 学习曲线 | 不自研 | — |
| 20 | **性能基准** | GoogleBenchmark | 业界标准 | nonius、picobench | — | 不自研 | — |
| 21 | **日志** | spdlog 1.12+ | 异步 batch 极快 | glog、boost.log、quill | — | 1 月（不自研） | — |
| 22 | **格式化** | fmtlib | C++20 std::format 起源 | std::format | 编译器支持参差 | 不自研 | — |
| 23 | **错误处理** | tl::expected（→ std::expected C++23） | 强制错误路径 | 异常、boost::outcome | C++23 polyfill | 不自研 | — |
| 24 | **压缩** | Zstandard | 高压缩比 + 快 | LZ4、zlib | — | 不自研 | 用于 .mycad 容器 |
| 25 | **嵌入式 DB** | SQLite 3.x | 业界事实标准 | DuckDB | — | 不自研 | — |

### 4.1.5 静态分析与代码质量

| # | 决策项 | 推荐选型 | 核心理由 | 备注 |
|---|---|---|---|---|
| 26 | **静态分析** | clang-tidy + cppcheck + IWYU | 覆盖完整 | VS 内置 clang-tidy |
| 27 | **代码格式化** | clang-format | 强制统一 | `.clang-format` + pre-commit |
| 28 | **EditorConfig** | `.editorconfig` | VS 自动识别 | — |
| 29 | **内存检测** | AddressSanitizer + LeakSanitizer + UBSan | Clang/GCC 内置 | CI 单独 sanitizer 构建 |
| 30 | **线程检测** | ThreadSanitizer | 数据竞争事实标准 | CI 专门 job |

### 4.1.6 构建、CI、发布

| # | 决策项 | 推荐选型 | 核心理由 | 备注 |
|---|---|---|---|---|
| 31 | **CI/CD** | GitHub Actions | 开源仓库免费 + 三平台 runner | OCCT 大型构建用自托管 runner（远期） |
| 32 | **依赖锁定** | vcpkg.json + baseline + lockfile | 可复现构建 | — |
| 33 | **Issue 管理** | GitHub Issues + Projects | 与代码仓库零摩擦 | 标签体系早期规划 |
| 34 | **Linux 发布** | AppImage（早期）+ Flatpak（远期） | AppImage 单文件易分发 | — |
| 35 | **Windows 发布** | ZIP portable（早期）+ MSIX（远期） | 早期不做安装器 | — |
| 36 | **macOS 发布** | .app + DMG + 公证 | 标准方案 | 苹果开发者 $99/年 |
| 37 | **崩溃报告** | sentry-native（远期） | 业界标准 | 早期不部署 |

### 4.1.7 文档、协作

| # | 决策项 | 推荐选型 | 核心理由 | 备注 |
|---|---|---|---|---|
| 38 | **API 文档** | Doxygen + Sphinx + Breathe | 工具链成熟 | — |
| 39 | **commit 规范** | Conventional Commits | 自动 changelog | 通过 commitlint 强制 |
| 40 | **本地化** | Qt LinguistTools | 与 Qt 集成 | 翻译协作可选 Crowdin |
| 41 | **Tracy 性能分析** | Tracy 0.10+ | 实时火焰图 | 推荐用于 CAD 调试 |

### 4.1.8 AI 相关

| # | 决策项 | 推荐选型 | 核心理由 | 备注 |
|---|---|---|---|---|
| 42 | **AI 模型推理** | ONNX Runtime + llama.cpp | 跨框架 + 私有化关键 | 模型许可逐一审 |
| 43 | **AI Adapter（云端 LLM）** | OpenAI 兼容 API + 自家 Adapter | 抹平多家 API 差异 | Claude/GPT/DeepSeek/通义可切换 |

---

## 4.2 协议许可证矩阵（Phase 1 起每季度审一次）

| 库 | 协议 | 与 myCad LGPL-3.0 兼容 | 商业插件可用 | 备注 |
|---|---|---|---|---|
| OpenCASCADE | LGPL-2.1 | ✅ | ✅（动态链接） | [ADR-0004](../adr/ADR-0004-occt-as-geometry-kernel.md) |
| Qt 6 | LGPL-3.0 / GPL-3.0 / 商业 | ✅ | ✅（动态链接） | 闭源插件需 LGPL 动态边界 |
| EnTT | MIT | ✅ | ✅ | [ADR-0007](../adr/ADR-0007-entt-as-ecs.md) |
| PlaneGCS | LGPL-2.1 | ✅ | ✅（动态） | 从 FreeCAD 提取 |
| Eigen | MPL-2.0 | ✅ | ✅ | 文件级弱传染 |
| FlatBuffers | Apache-2.0 | ✅ | ✅ | — |
| spdlog | MIT | ✅ | ✅ | — |
| fmt | MIT | ✅ | ✅ | — |
| Catch2 | BSL-1.0 | ✅ | ✅（仅测试用） | — |
| pybind11 | BSD-3 | ✅ | ✅ | — |
| nlohmann/json | MIT | ✅ | ✅ | — |
| Zstandard | BSD-3 + GPL-2.0（双） | ✅ | ✅ | — |
| SQLite | Public Domain | ✅ | ✅ | — |
| Tracy | BSD-3 | ✅ | ✅（仅 dev 用） | — |

---

## 4.3 重点决策的扩展说明

### 4.3.1 Visual Studio 2022 是个人开发者最优解

**为什么不是 VSCode**：CAD 调试场景下，VS 的 Watch / Memory / Threads 视图远超 VSCode；OCCT 这种深度模板代码在 VSCode 上 IntelliSense 经常失灵。

**为什么不是 CLion**：CLion 优秀，但 Windows + MSVC + OCCT 的调试体验略弱于 VS；个人开发者 CLion 年费 $99 vs VS Community 免费。

**完整论证**：[§八 Visual Studio 工具链](./08-vs-toolchain.md) + [ADR-0008](../adr/ADR-0008-visual-studio-toolchain.md)。

### 4.3.2 几何内核：OpenCASCADE 是唯一选项

OCCT 的接受不是"最优解"，是"无替代品"。CGAL 缺 STEP，Open NURBS 仅曲面，自研需 50-100 人月。

因此：
- 需要在所有架构层面**做好"OCCT 不可避免地泄漏"的准备**
- IGeometryPort 抽象不是为了"更换内核"，而是为了"控制 OCCT 的影响范围"
- 长期为自研留路径（[§九](./09-self-host-strategy.md)）

### 4.3.3 GUI 框架：Qt 几乎也是唯一现实选项

- wxWidgets：API 陈旧，OpenGL 集成弱
- Dear ImGui：即时模式不适合复杂 CAD
- Slint：年轻，生态薄
- Electron + 后端 C++：双语言地狱

Qt 的代价：
- LGPL 商业化需动态链接
- 学习曲线（信号槽、MOC、Layout）

### 4.3.4 包管理：必须用 vcpkg（不是 Conan）

vcpkg 对 OCCT 与 Qt 的 portfile 质量明显高于 Conan：

| 库 | vcpkg | Conan |
|---|---|---|
| OCCT | 官方 portfile | 社区 recipe，常滞后 |
| Qt 6 | 官方 portfile，组件化好 | qt-conan 配置极复杂 |

OCCT + Qt 是 myCad 的两大基石依赖，谁能搞顺这两个就该选谁 — **vcpkg 胜出**。

加之 Visual Studio 2022 17.6+ 内置 vcpkg 集成 → 与主 IDE 协同最佳。

### 4.3.5 渲染 API：OpenGL 4.5 而非 Vulkan

CAD 不是游戏。瓶颈在 BRep → Mesh、LOD、拓扑选择（CPU 端），不在 draw call 数量。

Vulkan 的隐藏成本：
- shader 必须用 SPIR-V
- 同步原语易写错
- 验证层调试门槛高

长期演进路径：通过 IRenderPort 抽象，远期引入 wgpu 后端覆盖 macOS Metal + Web WASM。

### 4.3.6 错误处理：std::expected 是定海神针

CAD 行业对异常普遍持负面态度：
- 大量"业务级失败"用异常表达 = 滥用
- 异常栈展开在 BRep 计算热路径上有性能影响
- OCCT 自身用混合方案，调用方难统一

`std::expected` 优势：
- 编译期强制处理错误路径（`[[nodiscard]]`）
- 错误信息可携带丰富诊断
- 与 DDD 业务错误天然对齐
- 与函数式风格的 monadic 操作兼容

C++23 之前用 [tl::expected](https://github.com/TartanLlama/expected) polyfill。

### 4.3.7 性能分析：Tracy 优先于 perf

CAD 性能问题大多是"用户操作触发的延迟"，需要：
- 实时观察
- 场景级关联
- 跨平台一致

`perf` / VTune 偏向"录制 + 离线分析"，对交互式 CAD 调试不够友好。

互补：Tracy 找问题区域，perf/VTune 做深度分析。

---

## 4.4 决策清单的维护流程

| 操作 | 何时执行 | 由谁 |
|---|---|---|
| 新增决策 | 引入新依赖时 | Claude Code 起草 + 人工 accept |
| 修改决策 | 替换某项依赖时 | 写新 ADR (Status: Supersedes ADR-NNNN) |
| 升级版本 | vcpkg baseline 升级时 | DeepSeek 跑回归 + Claude 审查 |
| 弃用决策 | 决策失效但未替换 | 标记 Status: Deprecated + 后续 ADR 跟进 |
| 长期主权指数评估 | 每年 Phase 末 | 维护者 |

---

> **最后修订**：2026-05（首版整合 + 增加 IDE 行 + 自研路径列）
