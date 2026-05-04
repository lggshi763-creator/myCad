# §八 Visual Studio 工具链

> 本章是 §六 路线图的工具层附录。Visual Studio 2022（社区版即可）是 myCad 的**主 IDE 与默认构建环境**。所有 sprint 任务的"开发体验"都以 VS 为基线设计。
>
> 决策依据：[ADR-0008 — Visual Studio 2022 作为主 IDE 与构建环境](../adr/ADR-0008-visual-studio-toolchain.md)

## 8.1 为什么是 Visual Studio 2022

### 8.1.1 评估维度对比

| 维度 | Visual Studio 2022 | VSCode + clangd | CLion | Qt Creator |
|---|---|---|---|---|
| **C++ 智能感知** | 业界最强（含 IntelliCode AI 补全） | 良（依赖 clangd） | 优秀 | 良 |
| **CMake 一等支持** | 原生（File → Open → CMake...） | 需扩展 | 原生 | 弱 |
| **vcpkg 集成** | 原生（VS 2022 17.6+ 内置 vcpkg） | 手动 | 手动 | 手动 |
| **MSVC 编译器** | 内置 | 需单独安装 | 需单独安装 | 需单独安装 |
| **调试器** | Windows 上业界最佳（含时间旅行调试） | 良 | 优秀 | 良 |
| **Catch2 测试探索** | Test Explorer 自动发现 | 需扩展 | 需配置 | 弱 |
| **Qt 工程支持** | Qt VS Tools 扩展 | 弱 | 良 | 优秀（自家） |
| **OpenCASCADE 调试** | 良（PDB 支持成熟） | 受限 | 良 | 良 |
| **远程 Linux 开发** | WSL/SSH 远程模式成熟 | 优秀（Remote Containers） | 优秀 | 弱 |
| **价格** | 社区版免费（个人/开源/教育） | 免费 | $99/年（开源免费） | 免费 |

### 8.1.2 关键论据（为何 VS 而非其他）

**vs VSCode**：CAD 调试场景下，VS 的 Watch / Locals / Memory / Threads 视图远超 VSCode；OCCT 这种深度模板代码在 VSCode 上 IntelliSense 经常失灵。

**vs CLion**：CLion 优秀，但 Windows 平台 MSVC + OCCT 调试体验略弱于 VS；个人开发者预算下 VS Community 免费是优势。

**vs Qt Creator**：Qt Creator 对 Qt 的体验最好，但对纯 C++ 大型项目（CAD 内核 + ECS + OCCT）的支持远弱于 VS。

### 8.1.3 跨平台兼容承诺

虽然 VS 是 Windows 主，但 myCad 必须在 Linux/macOS 也能编译 — 通过 CMake + vcpkg 实现。Linux/macOS 开发者可使用 VSCode 或 CLion，**项目不强制 IDE**。VS 是"主推荐"而非"唯一"。

CI 上三平台均跑通，杜绝 "在 VS 上 work，在 Linux CI 挂"的问题。

---

## 8.2 工作站初始化（一次性配置）

### 8.2.1 安装 Visual Studio 2022

**版本**：Community（免费，个人/开源足够）或 Professional/Enterprise（如有授权）

**安装时勾选的工作负载**：
- ✅ **使用 C++ 的桌面开发**（Desktop development with C++）
  - 子组件：MSVC v143、Windows 11 SDK、CMake tools for Windows、C++ 核心功能、Test Adapter for Catch2
- ✅ **使用 C++ 的 Linux 和嵌入式开发**（Linux and embedded development with C++）
  - 用于 WSL 远程编译验证
- ✅ **使用 C++ 的游戏开发**（Game development with C++）
  - 仅安装其中的 "C++ 性能分析工具"（diagnostic tools 增强）

**单独勾选的组件**：
- ✅ 适用于 Windows 的 C++ Clang 编译器（用于 clang-tidy / clang-format）
- ✅ 适用于 v143 生成工具的 C++ Clang-cl
- ✅ Git for Windows
- ✅ GitHub Extension for Visual Studio

**保存安装清单为 `.vsconfig`**：项目根目录提供 `tools/vs2022.vsconfig`，新人 import 即可一键配置。

### 8.2.2 vcpkg 配置

VS 2022 17.6+ 内置 vcpkg；**仍建议**单独 clone 一份方便升级：

```powershell
# 推荐位置：C:\dev\vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
C:\dev\vcpkg\vcpkg integrate install
# 设置环境变量
[Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\dev\vcpkg', 'User')
```

**项目内**用 manifest 模式（`vcpkg.json`），不用全局 install — 保证可复现：

```json
{
  "name": "mycad",
  "version-string": "0.0.1",
  "dependencies": [
    "opencascade",
    { "name": "qtbase", "default-features": false, "features": ["gui", "widgets"] },
    "qttools",
    "entt",
    "eigen3",
    "spdlog",
    "fmt",
    "flatbuffers",
    "catch2",
    "benchmark",
    "tl-expected",
    "nlohmann-json",
    "tomlplusplus",
    "zstd",
    "sqlite3"
  ],
  "builtin-baseline": "<git-sha-here>"
}
```

### 8.2.3 必装 VS 扩展

| 扩展 | 用途 | 强制度 |
|---|---|---|
| **Qt Visual Studio Tools** | Qt 项目模板、`.qrc` / `.ts` 编辑、designer 集成 | ★★★ |
| **Test Adapter for Catch2** | Test Explorer 显示 Catch2 测试用例 | ★★★ |
| **Markdown Editor v2** | 编辑 `docs/**/*.md` 时的预览 | ★★ |
| **GitHub Copilot** | AI 行内补全（与 Claude/DeepSeek 互补） | ★★ |
| **Clang Power Tools** | clang-tidy / clang-format 集成 | ★★ |
| **CMake Tools** | （VS 2022 已内置 CMake 支持，此扩展用于增强） | ★ |
| **VsVim** | 习惯 vim 键位者用 | ★（口味） |

### 8.2.4 Git 配置

```powershell
# 全局
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
git config --global core.autocrlf true   # Windows 行尾
git config --global core.longpaths true  # OCCT/Qt 路径长

# 项目根（首次 clone 后）
git config core.hooksPath .githooks       # 启用项目自带 hook
```

---

## 8.3 项目打开方式（CMake 直开）

myCad 不使用 `.vcxproj` / `.sln` 文件 — **统一用 CMake**，让所有平台/IDE 用同一份构建脚本。

### 8.3.1 推荐流程

1. **打开 VS 2022**
2. **File → Open → CMake...** → 选 `E:\myCad\CMakeLists.txt`
3. VS 自动识别 `CMakePresets.json`，列出可用 preset
4. 选 `vs2022-x64-debug`（或 release / asan / ubsan）
5. 等待 CMake 配置完成（首次 5-15 分钟，含 vcpkg 依赖编译）
6. **Solution Explorer** 切到 **CMake Targets View** → 看到所有 target
7. 设置启动项目（右键 mycad_app → "设置为启动项目"）
8. F5 调试运行

### 8.3.2 CMakePresets.json 设计

项目根提供 4 个 preset：

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "vs2022-x64-debug",
      "displayName": "VS 2022 x64 Debug",
      "generator": "Visual Studio 17 2022",
      "architecture": "x64",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "VCPKG_TARGET_TRIPLET": "x64-windows",
        "MYCAD_ENABLE_TESTING": "ON",
        "MYCAD_ENABLE_BENCHMARKS": "OFF"
      }
    },
    {
      "name": "vs2022-x64-release",
      "inherits": "vs2022-x64-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "MYCAD_ENABLE_BENCHMARKS": "ON"
      }
    },
    {
      "name": "vs2022-x64-asan",
      "inherits": "vs2022-x64-debug",
      "displayName": "VS 2022 x64 + AddressSanitizer",
      "cacheVariables": {
        "MYCAD_ENABLE_ASAN": "ON"
      }
    },
    {
      "name": "vs2022-x64-ubsan",
      "inherits": "vs2022-x64-debug",
      "cacheVariables": {
        "MYCAD_ENABLE_UBSAN": "ON"
      }
    }
  ],
  "buildPresets": [
    { "name": "vs2022-x64-debug",   "configurePreset": "vs2022-x64-debug" },
    { "name": "vs2022-x64-release", "configurePreset": "vs2022-x64-release" }
  ],
  "testPresets": [
    {
      "name": "vs2022-x64-debug",
      "configurePreset": "vs2022-x64-debug",
      "output": { "outputOnFailure": true }
    }
  ]
}
```

`CMakeUserPresets.json`（git ignored）允许个人覆盖（如自定义 `VCPKG_ROOT`）。

### 8.3.3 命令行替代（CI 与脚本场景）

```powershell
# 配置
cmake --preset vs2022-x64-debug

# 构建
cmake --build --preset vs2022-x64-debug --target mycad_app

# 测试
ctest --preset vs2022-x64-debug
```

---

## 8.4 调试工作流

### 8.4.1 启动调试（F5）

- **断点**：F9 切换；条件断点支持 lambda 表达式
- **数据断点**：右键变量 → "在以下条件更改时中断" — CAD 内核 BUG 定位神器
- **跟踪点（Tracepoint）**：右键断点 → "操作" → 输出表达式而不暂停 — 比 `printf` 调试优雅 10 倍
- **时间旅行调试（TTD，Enterprise 版）**：录制后可前后回放执行 — OCCT 偶发 bug 定位利器

### 8.4.2 自定义 natvis 可视化

C++ 复杂模板类型（如 `std::variant`、Eigen 矩阵、OCCT `Handle`）默认显示信息有限。在 `tools/visualizers/mycad.natvis` 中提供：

```xml
<!-- 让 BRepHandle 显示为 "BRepHandle{42, valid}" 而非裸数字 -->
<Type Name="mycad::domain::BRepHandle">
  <DisplayString Condition="id == 0">invalid</DisplayString>
  <DisplayString>BRepHandle{{ id={id} }}</DisplayString>
</Type>

<!-- 让 EventId (ULID) 显示为可读字符串 -->
<Type Name="mycad::domain::EventId">
  <DisplayString>ULID{{ {value,sb} }}</DisplayString>
</Type>

<!-- Sketch 聚合根显示实体数 / 约束数 -->
<Type Name="mycad::domain::sketch::Sketch">
  <DisplayString>Sketch{{ id={id_}, entities={entities_._Mypair._Myval2._Mylast - entities_._Mypair._Myval2._Myfirst}, version={version_.value} }}</DisplayString>
</Type>
```

natvis 文件随项目 commit，所有人受益。

### 8.4.3 OpenCASCADE 调试源码下载

vcpkg 默认编译 OCCT 不带源码符号。为了能 step into OCCT：

```powershell
# 在 vcpkg 配置中加入 source layout（项目 vcpkg-configuration.json）
{
  "default-registry": { "kind": "git", "repository": "...", "baseline": "..." },
  "registries": [],
  "overlay-triplets": ["./vcpkg/triplets"]
}

# 自定义 triplet 启用源码安装
# vcpkg/triplets/x64-windows-with-source.cmake
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_BUILD_TYPE debug)
set(VCPKG_KEEP_BUILD_TREES ON)  # 保留源码以便 step into
```

VS 调试器自动从 PDB 找到 OCCT 源码路径。

---

## 8.5 测试集成（Test Explorer）

### 8.5.1 Catch2 自动发现

安装 **Test Adapter for Catch2** 扩展后：

1. 构建一次项目
2. 打开 Test → Test Explorer
3. 自动列出所有 `TEST_CASE(...)`
4. 双击运行单个用例 / Run All Tests / Debug Selected

### 8.5.2 推荐 CMake 测试结构

```cmake
# tests/CMakeLists.txt
find_package(Catch2 3 REQUIRED)
include(Catch)

add_executable(mycad_unit_tests
    sketch_test.cpp
    feature_test.cpp
    eventstore_test.cpp
)
target_link_libraries(mycad_unit_tests
    PRIVATE
        Catch2::Catch2WithMain
        mycad::domain
        mycad::infrastructure
)

# 让 ctest 与 Test Explorer 都能发现
catch_discover_tests(mycad_unit_tests
    REPORTER junit
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-reports
)
```

### 8.5.3 GoogleBenchmark 单独 target

性能测试不放入默认 ctest（避免阻塞日常开发）：

```cmake
if(MYCAD_ENABLE_BENCHMARKS)
  add_executable(mycad_benchmarks
    sketch_solver_bench.cpp
    eventstore_bench.cpp
  )
  target_link_libraries(mycad_benchmarks PRIVATE benchmark::benchmark mycad::domain)
endif()
```

VS 中作为独立启动项手动运行。

---

## 8.6 静态分析与代码风格

### 8.6.1 clang-format

项目根 `.clang-format`：

```yaml
BasedOnStyle: LLVM
Language: Cpp
Standard: c++20

ColumnLimit: 100
IndentWidth: 4
UseTab: Never

AccessModifierOffset: -4
AlwaysBreakTemplateDeclarations: Yes
NamespaceIndentation: None
PointerAlignment: Left
ReferenceAlignment: Left

IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^"mycad/'
    Priority: 1
  - Regex: '^"'
    Priority: 2
  - Regex: '^<.*\.(h|hpp)>'
    Priority: 3
  - Regex: '^<[a-z_]+>$'
    Priority: 4
```

VS 自动应用：Tools → Options → Text Editor → C/C++ → Code Style → Formatting → 启用 clang-format。

### 8.6.2 clang-tidy

项目根 `.clang-tidy`：

```yaml
Checks: >
  -*,
  bugprone-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic,
  -cppcoreguidelines-pro-bounds-array-to-pointer-decay

WarningsAsErrors: bugprone-*

HeaderFilterRegex: '.*/mycad/.*\.hpp$'
```

VS 自动运行：Project Properties → Code Analysis → Clang-Tidy → 启用。

### 8.6.3 EditorConfig

项目根 `.editorconfig`（VS 自动识别）：

```ini
root = true

[*]
indent_style = space
indent_size = 4
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true
charset = utf-8

[*.{cpp,hpp,h,c,fbs}]
indent_size = 4

[CMakeLists.txt]
indent_size = 2

[*.md]
indent_size = 2
trim_trailing_whitespace = false  # Markdown 双空格换行
```

---

## 8.7 性能分析（VS 内置 + Tracy）

### 8.7.1 VS 性能分析器

Debug → Performance Profiler（Alt+F2）：
- **CPU Usage**：函数级火焰图
- **Memory Usage**：堆分配热点
- **Instrumentation**：精确函数调用计数（用于回归测试）
- **GPU Usage**：DirectX/OpenGL 调用追踪（CAD 渲染优化用）

### 8.7.2 Tracy（推荐用于 Real-time Profiling）

[Tracy](https://github.com/wolfpld/tracy) 是 CAD 调试的杀手工具：

```cpp
// 在代码中插桩
#include <tracy/Tracy.hpp>

void GeometryRebuildSystem::update() {
    ZoneScoped;  // 自动以函数名命名
    // ...
    {
        ZoneScopedN("OCCT Boolean");  // 自定义子区域
        result = occt.booleanUnion(a, b);
    }
}
```

VS 中：
1. 用 vcpkg 装 `tracy` 包
2. 启动 myCad → 启动 `tracy-server.exe` → 自动连接
3. 实时火焰图 + 各种聚合视图

---

## 8.8 插件开发体验

### 8.8.1 插件项目模板

`tools/plugin-template/` 提供一个最小插件示例：

```
plugin-template/
├── CMakeLists.txt
├── src/
│   └── HelloPlugin.cpp
├── CMakePresets.json
└── README.md
```

`CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.25)
project(hello_plugin VERSION 0.0.1)

find_package(mycad-sdk REQUIRED)

add_library(hello_plugin SHARED src/HelloPlugin.cpp)
target_link_libraries(hello_plugin PRIVATE mycad::sdk)
target_compile_features(hello_plugin PRIVATE cxx_std_20)

# 让插件 dll 输出到 myCad 插件目录
set_target_properties(hello_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${MYCAD_PLUGIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MYCAD_PLUGIN_DIR}"
)
```

VS 直接 Open Folder → 5 分钟跑通"hello world 插件出现在菜单"。

### 8.8.2 插件调试

主程序（mycad_app）启动时加载插件 → 在主程序的 VS 实例中：
1. Project Properties → Debugging → Working Directory = myCad 安装目录
2. F5 启动主程序
3. 主程序加载插件后，自动加载插件的 PDB
4. 在插件代码中下断点 → 命中

---

## 8.9 Python 插件 / 嵌入式 Python 工作流

### 8.9.1 Python 工具链

VS 2022 自带 Python 工作负载（可选安装）：
- 推荐独立安装 Python 3.11.x（与 vcpkg 的 pybind11 匹配）
- 装在 `C:\Python311\`，环境变量 `PYTHONHOME=C:\Python311`

### 8.9.2 pybind11 + Visual Studio

vcpkg 安装 pybind11 → CMake 中：

```cmake
find_package(Python3 3.11 EXACT REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)

# C++ 端：嵌入 Python
target_link_libraries(mycad_app PRIVATE Python3::Python pybind11::embed)

# Python 端：编译 Python 模块（如果有）
pybind11_add_module(mycad_py module.cpp)
```

### 8.9.3 Python 调试（混合模式）

VS 支持 C++/Python 混合调试：
1. Project Properties → Debugging → Debugger Type = "Auto" 或 "Native + Python"
2. 在 C++ 与 Python 代码都能下断点
3. 调用栈在两个语言间无缝切换

---

## 8.10 协作工作流

### 8.10.1 Git 集成

VS 2022 内置 Git 工具栏支持：
- 文件改动自动标注（gutter color）
- Branch / Commit / Pull Request 视图（Team Explorer）
- 与 GitHub Copilot 配合可生成 commit message

但日常仍推荐**命令行 Git** + VS 仅用于查看 diff —— 命令行更可控。

### 8.10.2 GitHub Copilot 与 Claude / DeepSeek 的关系

| 工具 | 场景 | 在 VS 中触发 |
|---|---|---|
| **GitHub Copilot** | 行内补全（写函数体时） | 自动 |
| **Claude Code** | 大块设计 / 审查 / 文档 | 单独 CLI 或 Web |
| **DeepSeek** | 整文件实现 / 算法 | 单独 Web / API |

三者不冲突：Copilot 是"打字加速器"，Claude 是"建筑师"，DeepSeek 是"高产工人"。

### 8.10.3 vcpkg + 团队协作（远期）

当贡献者增加：
- `vcpkg.json` 锁 baseline → 所有人构建相同依赖版本
- vcpkg binary cache（开源 GitHub Actions cache）→ 加速 CI 与新人首次构建
- `vcpkg-configuration.json` 中可加私有 registry（商业插件依赖）

---

## 8.11 故障排查 Cheatsheet

| 症状 | 可能原因 | 解决 |
|---|---|---|
| CMake 配置一直卡在 vcpkg | 网络问题 / 首次依赖下载 | 设代理 + 等（OCCT 编译可达 30 min） |
| IntelliSense 一直 "loading" | OCCT 头文件巨大 | Tools → Options → Text Editor → C/C++ → Advanced → Disable Database 加速 |
| Test Explorer 看不到 Catch2 测试 | Test Adapter 未装 / 未构建 | 装扩展 + Build All |
| F5 启动后立即崩溃 | OCCT/Qt DLL 找不到 | Working Directory 配置错；或用 `windeployqt.exe` 部署 Qt DLL |
| Asan 报告堆腐败但代码看着没问题 | Asan + OCCT 已知误报 | 用 ASAN_OPTIONS=detect_container_overflow=0 |
| Qt MOC 找不到 | CMakePresets 未启用 AUTOMOC | `set(CMAKE_AUTOMOC ON)` |
| pybind11 链接错 Python lib | Debug 配置链接了 release Python | 用 Python3_FIND_REGISTRY=NEVER 强制指定 |

---

## 8.12 与其他平台开发者的兼容

虽然 VS 是主推荐，但项目支持其他工具链。维护原则：

| 平台 | 推荐工具 | 必须保证 |
|---|---|---|
| Windows | Visual Studio 2022 | CMake + Test Explorer + clang-format 全部工作 |
| Linux | VSCode + clangd / CLion | CMake + ctest + clang-tidy 全部工作 |
| macOS | CLion / VSCode | CMake + ctest + clang-tidy 全部工作 |

CI 矩阵覆盖三平台，杜绝"VS 可编 Linux 不可编"。

---

## 8.13 维护责任

| 工件 | 维护者 | 更新频率 |
|---|---|---|
| `.vsconfig`（VS 安装清单） | 项目维护者 | 仅在引入新工具负载时 |
| `CMakePresets.json` | 项目维护者 | 引入新 build 配置时 |
| `.clang-format` / `.clang-tidy` | 项目维护者 | 风格调整或新规则启用 |
| `tools/visualizers/mycad.natvis` | 任何贡献者 | 引入复杂类型时建议补充 |
| 本文档 | 维护者 + 贡献者 | VS 版本升级或工具链变更 |

---

> **最后修订**：2026-05（首次创建，对应 ADR-0008）
